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
		bool annotate(const Argument &) const;
		void getAnalysisUsage(AnalysisUsage &) const final;
		bool runOnModule(Module &) override final;
		void print(raw_ostream &, const Module *) const;
	private:
		//map from function name and argument number to whether or not that argument gets annotated
		typedef map<pair<string, int>, Answer> AnnotationMap;
		AnnotationMap annotations;
		bool getAnswer(const Argument &) const;
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
	const Function &func = *arg.getParent();
	pair<string, int> key = make_pair(func.getName(), arg.getArgNo());
	AnnotationMap::const_iterator found = annotations.find(key);
	return found != annotations.end() && found->second == NULL_TERMINATED;
}


void NullAnnotator::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<FindSentinels>();
}

bool NullAnnotator::getAnswer(const Argument &arg) const {
	const Function &func = *arg.getParent();
	pair<string, int> key = make_pair(func.getName(), arg.getArgNo());
	AnnotationMap::const_iterator found = annotations.find(key);
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
			const unordered_map<const BasicBlock *, ArgumentToBlockSet> &functionChecks = findSentinels.getResultsForFunction(&func);
			for (const Argument &arg : func.getArgumentList()) {
				pair<string, int> key = make_pair(func.getName(), arg.getArgNo());
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
					//Answer report = getAnswer(arg)
					//if (report == NULL_TERMINATED){
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
				if (hasLoopWithSentinelCheck(functionChecks, arg)) {
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
