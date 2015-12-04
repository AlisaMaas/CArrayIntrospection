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
	static const RegisterPass<SymbolicRangeTest> registration("sra-test-easily",
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
        for (Function &func : module) {
            if (func.isDeclaration()) continue;
            errs() << "Processing " << func.getName().str() << "\n";
            for (BasicBlock &block : func) {
                for (Instruction &inst : block) {
                    if (inst.getType()->isIntegerTy()) {
                        const SymbolicRangeAnalysis &sra = getAnalysis<SymbolicRangeAnalysis>(func);
                        BasicBlock *placeHolder = BasicBlock::Create(module.getContext());
                        Value *symbolicIndexInst = sra.getRangeValuesFor(&inst, IRBuilder<>(placeHolder)).second;
                        SAGERange R = sra.getState(&inst);
                        errs() << "Address is " << &inst;
                        errs() << inst << " [" << R.getLower() << ", " << R.getUpper()
                    << "]\n";
                        if (symbolicIndexInst != nullptr)
                            errs() << "\t\tValue is " << *symbolicIndexInst << "\n";
                        delete placeHolder;
                    }
                }
            }
        }

	return false;
}


void SymbolicRangeTest::print(raw_ostream &, const Module *) const {

}
