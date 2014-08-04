#include "FindSentinels.hh"
#include "IIGlueReader.hh"

#include <llvm/IR/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Support/CallSite.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Use.h>
#include <tuple>
using namespace llvm;
using namespace std;

enum Answer{ DONT_CARE, NON_NULL_TERMINATED, NULL_TERMINATED };

namespace {
	class NullAnnotator : public ModulePass {
	public:
		NullAnnotator();
		static char ID;
		bool annotate(const Argument &) const;
		void getAnalysisUsage(AnalysisUsage &) const final;
		bool runOnModule(Module &) override final;
		void print(raw_ostream &, const Module *) const;
	private:
		//map from function name and argument number to whether or not that argument gets annotated
		typedef unordered_map<const Argument*, Answer> AnnotationMap;
		AnnotationMap annotations;
		unordered_map<const Function*, unordered_set<const CallInst*>> functionToCallSites;
		Answer getAnswer(const Argument &) const;
	};
	char NullAnnotator::ID;
}

static const RegisterPass<NullAnnotator> registration("null-annotator",
		"Determine whether and how to annotate each function with the null-terminated annotation",
		true, true);

bool existsNonOptionalSentinelCheck(const unordered_map<const BasicBlock *, ArgumentToBlockSet> checks, const Argument &arg);
bool hasLoopWithSentinelCheck(const unordered_map<const BasicBlock *, ArgumentToBlockSet> checks, const Argument &arg);

inline NullAnnotator::NullAnnotator()
: ModulePass(ID) { }

bool NullAnnotator::annotate(const Argument &arg) const {
	AnnotationMap::const_iterator found = annotations.find(&arg);
	return found != annotations.end() && found->second == NULL_TERMINATED;
}


void NullAnnotator::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<FindSentinels>();
}

Answer NullAnnotator::getAnswer(const Argument &arg) const {
	AnnotationMap::const_iterator found = annotations.find(&arg);
	return found == annotations.end() ? DONT_CARE : found->second;
}

bool NullAnnotator::runOnModule(Module &module) {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	const FindSentinels &findSentinels = getAnalysis<FindSentinels>();
	bool changed = true;
	bool firstTime = true;
	while (changed) {
		changed = false;
		for (const Function &func : module) {
			errs() << "About to get the map for this function\n";
			const unordered_map<const BasicBlock *, ArgumentToBlockSet> &functionChecks = findSentinels.getResultsForFunction(&func);
			for (const Argument &arg : iiglue.arrayArguments(func)) {
				errs() << "\tConsidering " << arg.getArgNo() << "\n";
				Answer oldResult = getAnswer(arg);
				errs() << "\tOld result: " << oldResult << '\n';
				if(oldResult == NULL_TERMINATED)
					continue;
				if (firstTime) {
					//process loops exactly once
					if (existsNonOptionalSentinelCheck(functionChecks, arg)) {
						errs() << "\tFound a non-optional sentinel check in some loop!\n";
						annotations[&arg] = NULL_TERMINATED;
						changed = true;
						continue;
					}
					for (auto I = inst_begin(func), E = inst_end(func); I != E; ++I) {
						const CallInst *call = dyn_cast<CallInst>(&*I);
						if (call) {
							functionToCallSites[&func].insert(call);
						}
					}
					errs() << "went through all the instructions and grabbed calls\n";
					errs() << "We found " << functionToCallSites[&func].size() << " calls\n";
				}
				//if we haven't yet continued, process evidence from callees.
				bool foundDontCare = false;
				bool foundNonNullTerminated = false;
				bool nextArgumentPlease = false;
				
				for (const CallInst *call : functionToCallSites[&func]) {
					const Argument * parameter = NULL;
					unsigned argNo = 0;
					bool foundArg = false;
					errs() << "About to iterate over the arguments to the call\n";
					for (unsigned i = 0; i < call->getNumArgOperands(); ++i) {
						errs() << "got one, about to call get\n";
						if (call->getArgOperand(i) == &arg){
							errs() << "hey, it matches!\n";
							argNo = i;
							foundArg = true;
							break;
						}
					}
					errs() << "Done looking through arguments\n";
					if (!foundArg) {
						continue;
					}
					for (const Argument &param : call->getCalledFunction()->getArgumentList()) {
						if (param.getArgNo() == argNo) {
							parameter = &param;
							break;
						}
					}
				
					Answer report = getAnswer(*parameter);
					if (report == NULL_TERMINATED) {
						errs() << "Marking NULL_TERMINATED\n";
						annotations[&arg] = NULL_TERMINATED;
						changed = true;
						nextArgumentPlease = true;
						break;
					}
					else if (report == NON_NULL_TERMINATED) {
						//maybe set/check a flag for error reporting
						foundNonNullTerminated = true;
					}
					else {
						//maybe set/check a flag for error reporting
						if(foundNonNullTerminated){
							errs() << "Found both DONT_CARE and NON_NULL_TERMINATED among callees.\n";
						}
						foundDontCare = true;
					}
				}
				if (nextArgumentPlease) {
					continue;
				}
				//if we haven't yet marked NULL_TERMINATED, might be NON_NULL_TERMINATED
				if (hasLoopWithSentinelCheck(functionChecks, arg)) {
					if (oldResult != NON_NULL_TERMINATED) {
						errs() << "Marking NOT_NULL_TERMINATED\n";
						annotations[&arg] = NON_NULL_TERMINATED;
						changed = true;
						if (foundDontCare)
							errs() << "Marking NULL_TERMINATED even though other calls say DONT_CARE.\n";
							//do error reporting stuff
						continue;
					}
				}
				//otherwise it stays as DONT_CARE for now.
			}
		}
		firstTime = false;
	}
	
	return false;
}


void NullAnnotator::print(raw_ostream &sink, const Module* module) const {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	for (const Function &func : *module) {
		for (const Argument &arg : iiglue.arrayArguments(func)) {
			sink << func.getName() << " with argument "<< arg.getArgNo() << " should " << (annotate(arg)?"":"not ");
			sink << "be annotated NULL_TERMINATED.\n";
		}
	}
}

bool existsNonOptionalSentinelCheck(unordered_map<const BasicBlock *, ArgumentToBlockSet> checks, const Argument &arg) {

	for (auto mapElements : checks) {
		const BasicBlock * const header = mapElements.first;
		const ArgumentToBlockSet &entry = checks.at(header);
		if(!entry.at(&arg).second)
			return true;
	}
	return false;
}

bool hasLoopWithSentinelCheck(unordered_map<const BasicBlock *, ArgumentToBlockSet> checks, const Argument &arg){
	for (auto mapElements : checks) {
		const BasicBlock * const header = mapElements.first;
		const ArgumentToBlockSet &entry = checks.at(header);
		if(!entry.at(&arg).first.empty())
			return true;
	}
	return false;
}