#include "FindSentinels.hh"
#include "IIGlueReader.hh"

#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/lambda/core.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/iterator_range.hpp>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/InstIterator.h>

using namespace boost;
using namespace boost::adaptors;
using namespace boost::algorithm;
using namespace llvm;
using namespace std;

enum Answer { DONT_CARE, NON_NULL_TERMINATED, NULL_TERMINATED };

namespace {
	class NullAnnotator : public ModulePass {
	public:
		// standard LLVM pass interface
		NullAnnotator();
		static char ID;
		void getAnalysisUsage(AnalysisUsage &) const final;
		bool runOnModule(Module &) override final;
		void print(raw_ostream &, const Module *) const;

		// access to analysis results derived by this pass
		bool annotate(const Argument &) const;

	private:
		// map from function name and argument number to whether or not that argument gets annotated
		typedef unordered_map<const Argument*, Answer> AnnotationMap;
		AnnotationMap annotations;
		typedef unordered_set<const CallInst*> CallInstSet;
		unordered_map<const Function*, CallInstSet> functionToCallSites;
		Answer getAnswer(const Argument &) const;
	};
	char NullAnnotator::ID;
}

static const RegisterPass<NullAnnotator> registration("null-annotator",
		"Determine whether and how to annotate each function with the null-terminated annotation",
		true, true);

bool existsNonOptionalSentinelCheck(const FindSentinels::FunctionResults &checks, const Argument &arg);
bool hasLoopWithSentinelCheck(const FindSentinels::FunctionResults &checks, const Argument &arg);

inline NullAnnotator::NullAnnotator()
	: ModulePass(ID) {
}

bool NullAnnotator::annotate(const Argument &arg) const {
	const AnnotationMap::const_iterator found = annotations.find(&arg);
	return found != annotations.end() && found->second == NULL_TERMINATED;
}


void NullAnnotator::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<FindSentinels>();
}

Answer NullAnnotator::getAnswer(const Argument &arg) const {
	const AnnotationMap::const_iterator found = annotations.find(&arg);
	return found == annotations.end() ? DONT_CARE : found->second;
}

bool NullAnnotator::runOnModule(Module &) {

	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();

	// collect calls in each function for repeated scanning later
	for (const Function &func : iiglue.arrayReceivers()) {
		const auto instructions =
			make_iterator_range(inst_begin(func), inst_end(func))
			| transformed([](const Instruction &inst) { return dyn_cast<CallInst>(&inst); })
			| filtered(boost::lambda::_1);
		functionToCallSites.emplace(&func, CallInstSet(instructions.begin(), instructions.end()));
		DEBUG(dbgs() << "went through all the instructions and grabbed calls\n");
		DEBUG(dbgs() << "We found " << functionToCallSites[&func].size() << " calls in " << func.getName() << '\n');
	}

	const FindSentinels &findSentinels = getAnalysis<FindSentinels>();
	bool firstTime = true;
	bool changed;

	do {
		changed = false;
		for (const Function &func : iiglue.arrayReceivers()) {
			DEBUG(dbgs() << "About to get the map for this function\n");
			const FindSentinels::FunctionResults &functionChecks = findSentinels.getResultsForFunction(&func);
			for (const Argument &arg : iiglue.arrayArguments(func)) {
				DEBUG(dbgs() << "\tConsidering " << arg.getArgNo() << "\n");
				Answer oldResult = getAnswer(arg);
				DEBUG(dbgs() << "\tOld result: " << oldResult << '\n');
				if (oldResult == NULL_TERMINATED)
					continue;
				if (firstTime) {
					// process loops exactly once
					if (existsNonOptionalSentinelCheck(functionChecks, arg)) {
						DEBUG(dbgs() << "\tFound a non-optional sentinel check in some loop!\n");
						annotations[&arg] = NULL_TERMINATED;
						changed = true;
						continue;
					}
				}
				// if we haven't yet continued, process evidence from callees.
				bool foundDontCare = false;
				bool foundNonNullTerminated = false;
				bool nextArgumentPlease = false;
				
				for (const CallInst &call : functionToCallSites[&func] | indirected) {
					unsigned argNo = 0;
					bool foundArg = false;
					DEBUG(dbgs() << "About to iterate over the arguments to the call\n");
					for (const unsigned i : irange(0u, call.getNumArgOperands())) {
						DEBUG(dbgs() << "got one, about to call get\n");
						if (call.getArgOperand(i) == &arg) {
							DEBUG(dbgs() << "hey, it matches!\n");
							argNo = i;
							foundArg = true;
							break;
						}
					}
					DEBUG(dbgs() << "Done looking through arguments\n");
					if (!foundArg) {
						continue;
					}
					auto formals = call.getCalledFunction()->getArgumentList().begin();
					advance(formals, argNo);
					const Argument &parameter = *formals;
				
					switch (getAnswer(parameter)) {
					case NULL_TERMINATED:
						DEBUG(dbgs() << "Marking NULL_TERMINATED\n");
						annotations[&arg] = NULL_TERMINATED;
						changed = true;
						nextArgumentPlease = true;
						break;

					case NON_NULL_TERMINATED:
						// maybe set/check a flag for error reporting
						foundNonNullTerminated = true;
						break;

					case DONT_CARE:
						// maybe set/check a flag for error reporting
						if (foundNonNullTerminated) {
							DEBUG(dbgs() << "Found both DONT_CARE and NON_NULL_TERMINATED among callees.\n");
						}
						foundDontCare = true;
						break;

					default:
						// should never happen!
						abort();
					}
				}
				if (nextArgumentPlease) {
					continue;
				}
				// if we haven't yet marked NULL_TERMINATED, might be NON_NULL_TERMINATED
				if (hasLoopWithSentinelCheck(functionChecks, arg)) {
					if (oldResult != NON_NULL_TERMINATED) {
						DEBUG(dbgs() << "Marking NOT_NULL_TERMINATED\n");
						annotations[&arg] = NON_NULL_TERMINATED;
						changed = true;
						if (foundDontCare) {
							DEBUG(dbgs() << "Marking NULL_TERMINATED even though other calls say DONT_CARE.\n");
							// do error reporting stuff
						}
						continue;
					}
				}
				// otherwise it stays as DONT_CARE for now.
			}
		}
		firstTime = false;
	} while (changed);

	return false;
}


void NullAnnotator::print(raw_ostream &sink, const Module *module) const {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	for (const Function &func : *module) {
		for (const Argument &arg : iiglue.arrayArguments(func)) {
			sink << func.getName() << " with argument " << arg.getArgNo() << " should " << (annotate(arg) ? "" : "not ");
			sink << "be annotated NULL_TERMINATED.\n";
		}
	}
}

bool existsNonOptionalSentinelCheck(const FindSentinels::FunctionResults &checks, const Argument &arg) {
	return any_of(checks | map_values,
		      [&](const ArgumentToBlockSet &entry) { return !entry.at(&arg).second; });
}

bool hasLoopWithSentinelCheck(const FindSentinels::FunctionResults &checks, const Argument &arg) {
	return any_of(checks | map_values,
		      [&](const ArgumentToBlockSet &entry) { return !entry.at(&arg).first.empty(); });
}
