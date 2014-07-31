#include "FindSentinels.hh"
#include "IIGlueReader.hh"

#include <llvm/IR/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Support/CallSite.h>
#include <llvm/Support/InstIterator.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/IR/Use.h>
#include <tuple>
using namespace llvm;
using namespace std;

enum Answer{DONT_CARE, NON_NULL_TERMINATED, NULL_TERMINATED};

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
		unordered_map<const Argument*, Answer> annotations;
		Answer getAnswer(const Argument &) const;
		unordered_map<const Function*, unordered_set<ImmutableCallSite*>> functionToCallSites;
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
	if (annotations.count(&arg)) {
		return annotations.at(&arg)== NULL_TERMINATED;
	}
	return false;
}


void NullAnnotator::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<FindSentinels>();
}

Answer NullAnnotator::getAnswer(const Argument &arg) const {
	if (annotations.count(&arg)){
		return annotations.at(&arg);
	}
	return DONT_CARE;
}

bool NullAnnotator::runOnModule(Module &module) {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	const FindSentinels &findSentinels = getAnalysis<FindSentinels>();
	bool changed = true;
	bool firstTime = true;
	while (changed) {
		changed = false;
		for (const Function &func : module) {
			const unordered_map<const BasicBlock *, ArgumentToBlockSet> functionChecks = findSentinels.getResultsForFunction(&func);
			for (const Argument &arg : func.getArgumentList()) {
				if (!iiglue.isArray(arg)) {
					continue;
				}
				bool oldResult = getAnswer(arg);
				if(oldResult == NULL_TERMINATED)
					continue;
				if (firstTime) {
					firstTime = false;
					//process loops exactly once
					if (existsNonOptionalSentinelCheck(functionChecks, arg)) {
						annotations[&arg] = NULL_TERMINATED;
						changed = true;
						continue;
					}
					for (auto I = inst_begin(func), E = inst_end(func); I != E; ++I) {
						ImmutableCallSite call(&*I);
						if (call) {
							functionToCallSites[&func].insert(&call);
						}
					}
				}
				//if we haven't yet continued, process evidence from callees.
				bool foundDontCare = false;
				bool foundNonNullTerminated = false;
				bool nextArgumentPlease = false;
				
				for (const ImmutableCallSite *call : functionToCallSites[&func]) {
					const Argument * parameter = NULL;
					unsigned argNo = 0;
					bool foundArg = false;
					for (auto AI = call->arg_begin(), E = call->arg_end(); AI != E; ++AI) {
						if (AI->get() == &arg){
							argNo = AI - AI->getUser()->op_begin();
							foundArg = true;
						}
					}
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
	
	}
	
	return false;
}


void NullAnnotator::print(raw_ostream &sink, const Module*) const {
	(void)sink;
}

bool existsNonOptionalSentinelCheck(unordered_map<const BasicBlock *, ArgumentToBlockSet> checks, const Argument &arg) {

	for (auto mapElements : checks) {
		const BasicBlock * const header = mapElements.first;
		ArgumentToBlockSet entry = checks.at(header);
		if(!entry.at(&arg).second)
			return true;
	}
	return false;
}

bool hasLoopWithSentinelCheck(unordered_map<const BasicBlock *, ArgumentToBlockSet> checks, const Argument &arg){
	for (auto mapElements : checks) {
		const BasicBlock * const header = mapElements.first;
		ArgumentToBlockSet entry = checks.at(header);
		if(!entry.at(&arg).first.empty()){
			return true;
		}
	}
	return false;
}
