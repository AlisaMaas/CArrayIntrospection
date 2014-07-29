#include "FindSentinels.hh"
#include "IIGlueReader.hh"
#include <llvm/PassManager.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;


namespace {
	class NullAnnotator : public ModulePass {
	public:
		NullAnnotator();
		static char ID;
		void getAnalysisUsage(AnalysisUsage &) const final;
		bool runOnModule(Module &) override final;
		void print(raw_ostream &, const Module *) const;
	};

	char NullAnnotator::ID;
}

static const RegisterPass<NullAnnotator> registration("null-annotator",
		"Determine whether and how to annotate each function with the null-terminated annotation",
		true, true);


inline NullAnnotator::NullAnnotator()
: ModulePass(ID)
{
}

void NullAnnotator::getAnalysisUsage(AnalysisUsage &usage) const
{
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<FindSentinels>();
}


bool NullAnnotator::runOnModule(Module &module) {
	(void) module;
	return false;
}


void NullAnnotator::print(raw_ostream &sink, const Module*) const {
	(void)sink;
}