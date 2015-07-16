#define DEBUG_TYPE "sra-test"
#include "SRA/SymbolicRangeAnalysis.h"

#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

using namespace llvm;
using namespace std;


namespace {
	class SymbolicRangeTest : public ModulePass {
	public:
		// standard LLVM pass interface
		SymbolicRangeTest();
		static char ID;
		void getAnalysisUsage(AnalysisUsage &) const final override;
		bool runOnModule(Module &) final override;
		void print(raw_ostream &, const Module *) const final override;
	};


	char SymbolicRangeTest::ID;
	static const RegisterPass<SymbolicRangeTest> registration("sra-test",
		"Playing around with the Symbolic Range Analysis tool.",
		true, true);
}



inline SymbolicRangeTest::SymbolicRangeTest()
	: ModulePass(ID) {
}


void SymbolicRangeTest::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<SymbolicRangeAnalysis>();
}




bool SymbolicRangeTest::runOnModule(Module &module) {
        const SymbolicRangeAnalysis &sra = getAnalysis<SymbolicRangeAnalysis>();
        for (Function &func : module) {
            errs() << "Processing " << func.getName().str() << "\n";
            for (BasicBlock &block : func) {
                for (Instruction &inst : block) {
                    if (!inst.getType()->isIntegerTy()) continue;
                    SAGERange R = sra.getState(&inst);
                    errs() << inst << "  [" << R.getLower() << ", " << R.getUpper()
                << "]\n";
                }
            }
        }
	return false;
}


void SymbolicRangeTest::print(raw_ostream &, const Module *) const {

}
