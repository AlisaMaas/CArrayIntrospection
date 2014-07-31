#include "FindSentinels.hh"
#include "IIGlueReader.hh"
#include <llvm/Support/InstIterator.h>
#include <llvm/IR/Module.h>
#include <llvm/PassManager.h>
#include <llvm/Support/raw_ostream.h>
#include <tuple>
using namespace llvm;
using namespace std;

enum Answer{DONT_CARE, NON_NULL_TERMINATED, NULL_TERMINATED};

namespace {
	class NullAnnotator : public ModulePass {
	public:
		NullAnnotator();
		static char ID;
		bool annotate(Function * func, Argument * arg) const;
		void getAnalysisUsage(AnalysisUsage &) const final;
		bool runOnModule(Module &) override final;
		void print(raw_ostream &, const Module *) const;
	private:
		//map from function name and argument number to whether or not that argument gets annotated
		map<pair<string, int>, Answer> annotations;
		bool getAnswer(Function * func, Argument * arg) const;
	};
	char NullAnnotator::ID;
}

static const RegisterPass<NullAnnotator> registration("null-annotator",
		"Determine whether and how to annotate each function with the null-terminated annotation",
		true, true);

bool existsNonOptionalSentinelCheck(unordered_map<BasicBlock const *, ArgumentToBlockSet> checks, Argument *arg);
bool hasLoopWithSentinelCheck(unordered_map<BasicBlock const *, ArgumentToBlockSet> checks, Argument *arg);

inline NullAnnotator::NullAnnotator()
: ModulePass(ID) { }

bool NullAnnotator::annotate(Function * func, Argument * arg) const {
	pair<string, int> key = make_pair(func->getName(), arg->getArgNo());
	if (annotations.find(key) != annotations.end()){
		return annotations.at(key)== NULL_TERMINATED;
	}
	return false;
}


void NullAnnotator::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<FindSentinels>();
}

bool NullAnnotator::getAnswer(Function * func, Argument * arg) const {
	pair<string, int> key = make_pair(func->getName(), arg->getArgNo());
	if (annotations.find(key) != annotations.end()){
		return annotations.at(key);
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
		for (Function * func = module.begin(); func != module.end(); ++func) {
			unordered_map<BasicBlock const *, ArgumentToBlockSet> functionChecks = findSentinels.getResultsForFunction(func);
			for (Argument* arg = func->arg_begin(); arg != func->arg_end(); ++arg) {
				pair<string, int> key = make_pair(func->getName(), arg->getArgNo());
				if (!iiglue.isArray(*arg)) {
					continue;
				}
				bool oldResult = getAnswer(func, arg);
				if (oldResult == NULL_TERMINATED)
					continue;
				if (firstTime) {
					firstTime = false;
					//process loops exactly once
					if (existsNonOptionalSentinelCheck(functionChecks, arg)) {
						annotations[key] = NULL_TERMINATED;
						changed = true;
						continue;
					}
				}
				//if we haven't yet continued, process evidence from callees.
				//bool foundDontCare = false;
				//bool foundNonNullTerminated = false;
				//for call : callees
				//for (auto I = inst_begin(*func), E = inst_end(*func); I != E; ++I) {
				//	ImmutableCallSite call(&*I);
					//Answer report = getAnswer(call.function(), arg)
					//if (report == NULL_TERMINATED) {
						//annotations[key] = NULL_TERMINATED;
						//changed = true;
						//continue;
					//}
					//else if (report == NON_NULL_TERMINATED) {
						//maybe set/check a flag for error reporting
						//foundNonNullTerminated = true;
					//}
					//else {
						//maybe set/check a flag for error reporting
						//foundDontCare = true;
					//}
				//}
				//if we haven't yet marked NULL_TERMINATED, might be NON_NULL_TERMINATED
				if (hasLoopWithSentinelCheck(functionChecks, arg)){
					if (oldResult != NON_NULL_TERMINATED) {
						annotations[key] = NON_NULL_TERMINATED;
						changed = true;
						//if (foundDontCare)
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

bool existsNonOptionalSentinelCheck(unordered_map<BasicBlock const *, ArgumentToBlockSet> checks, Argument *arg) {

	for (auto mapElements : checks) {
		const BasicBlock * const header = mapElements.first;
		ArgumentToBlockSet entry = checks.at(header);
		if(!entry.at(arg).second)
			return true;
	}
	return false;
}

bool hasLoopWithSentinelCheck(unordered_map<BasicBlock const *, ArgumentToBlockSet> checks, Argument *arg){
	for (auto mapElements : checks) {
		const BasicBlock * const header = mapElements.first;
		ArgumentToBlockSet entry = checks.at(header);
		if(!entry.at(arg).first.empty()){
			return true;
		}
	}
	return false;
}