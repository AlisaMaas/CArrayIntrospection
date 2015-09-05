#define DEBUG_TYPE "no-pointer-arithmetic"
#define TESTING 0
#include <fstream>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace llvm;
using namespace std;
namespace {
	class NoPointerArithmetic : public ModulePass {
		public:
			// standard LLVM pass interface
			NoPointerArithmetic();
			static char ID;
			void getAnalysisUsage(AnalysisUsage &) const final override;
			bool runOnModule(Module &) final override;
			void print(raw_ostream &, const Module *) const final override;
	};
	static const RegisterPass<NoPointerArithmetic> registration("no-pointer-arithmetic",
			"Replace pointer arithmetic with GEP instructions",
			true, false);
    static llvm::cl::opt<std::string>
        testOutputName("test-no-pointer-arithmetic",
        llvm::cl::Optional,
        llvm::cl::value_desc("filename"),
        llvm::cl::desc("Filename to write results to for regression tests"));

	char NoPointerArithmetic::ID;


	inline NoPointerArithmetic::NoPointerArithmetic()
		: ModulePass(ID) {
	}

	void NoPointerArithmetic::getAnalysisUsage(AnalysisUsage &) const {
	}


	bool NoPointerArithmetic::runOnModule(Module &module) {
		bool modified = false;
		for (Function &func : module) {
			for(Argument &arg : func.getArgumentList()) {
				if (arg.getType()->isPointerTy()) {
					for (User *user : arg.users()) {
						if (PHINode *node = dyn_cast<PHINode>(user)) {
							if (node->getNumIncomingValues() != 2) {
								DEBUG(dbgs() << "Found something close to pattern with other than 2 args\n");
								DEBUG(dbgs() << *node << "\n");
								continue;
							}
							int argument = 0;
							int other = 1;
							/*Value *toAdd = nullptr;
							BinaryOperator *oldAdd = nullptr;*/
							if (node->getIncomingValue(0) != &arg) {
								argument = 1;
								other = 0;
							}
							/*if (BinaryOperator *binOp = dyn_cast<BinaryOperator>(node->getIncomingValue(other))) {
								if (!binOp->getOpcode() != Instruction::Add) {
									errs() << "Not matching pattern because the non-argument was not an add\n";
									errs() << *node << "\n";
									errs() << "\t" << *binOp << "\n";
									continue; 
								}
								else {
									oldAdd = binOp;
									bool foundNode = false;
									for (int i = 0; i < 2; i++) {
										Value *operand = binOp->getOperand(i);
										if (operand == node) {
											foundNode = true;
										}
										else {
											toAdd = operand;
										}
									}
									if (!foundNode) {
										errs() << "Not matching pattern because the add didn't have the phi in it\n";
										errs() << *node << "\n";
										continue;
									}
								
								}
								modified = true;
								IntegerType* tInt = Type::getInt64Ty(context);
								PHINode *replacement = PHINode::Create(tInt, node->getNumIncomingValues(), "", node);
								replacement->addIncoming(ConstantInt::get(tInt, 0), node->getIncomingBlock(argument));
								replacement->addIncoming(BinaryOperator::Create(Instruction::Add, replacement, toAdd, "", oldAdd), node->getIncomingBlock(other));
								GetElementPtrInst *gep = GetElementPtrInst::Create(&arg, ArrayRef<Value*>(replacement), 
									"", &*node->getParent()->getFirstInsertionPt());
								errs() << "Replacing " << *node << "\n";
								errs() << "With " << *replacement << "\n";
								node->replaceAllUsesWith(gep);
							}
							else*/ if (GetElementPtrInst *gepi = dyn_cast<GetElementPtrInst>(node->getIncomingValue(other))){
								DEBUG(dbgs() << "Found a gep!\n");
								if (TESTING) gepi->dump();
								if (gepi->getPointerOperand() != node) {
									DEBUG(dbgs() << "Not matching pattern because the gep didn't have the pointer in it.\n");
									DEBUG(dbgs() << *gepi << "\n");
									continue;
								}
								else if (gepi->getNumIndices() != 1) {
									DEBUG(dbgs() << "Not matching pattern because there is more than one index.\n");
									DEBUG(dbgs() << *gepi << "\n");
									continue;
								}
								else {
								    DEBUG(dbgs() << *(*gepi->idx_begin())->getType() << "\n");
                                    if (TESTING) node->getParent()->getTerminator()->dump();

								    modified = true;
								    Type* tInt = (*gepi->idx_begin())->getType();
								    DEBUG(dbgs() << *tInt << "\n");
								    PHINode *addPHI = PHINode::Create(tInt, 2, "", node->getParent()->begin());
								    if (TESTING) node->getParent()->getTerminator()->dump();
								    BinaryOperator *add = BinaryOperator::CreateNSW(Instruction::Add, addPHI, *gepi->idx_begin(), "", gepi);
								    
								    DEBUG(dbgs() << "Add \n");
								    if (TESTING) { 
								        add->dump();
								        node->getParent()->getTerminator()->dump();
								    }

								    addPHI->addIncoming(ConstantInt::get(tInt, 0), node->getIncomingBlock(argument));
								    addPHI->addIncoming(add, node->getIncomingBlock(other));
								    DEBUG(dbgs() << "AddPHI: \n");
								    if (TESTING) addPHI->dump();
								    GetElementPtrInst *replacement = GetElementPtrInst::Create(
#if (1000 * LLVM_VERSION_MAJOR + LLVM_VERSION_MINOR) >= 3007
									    node->getType(),
#endif	// LLVM 3.7 or later
									    &arg, ArrayRef<Value*>(addPHI), "", node->getParent()->getFirstInsertionPt());
								    DEBUG(dbgs() << "Replacing " << *node << "\n");
								    DEBUG(dbgs() << "With " << *replacement << "\n");
								    node->replaceAllUsesWith(replacement);
								    node->eraseFromParent();
								}
								//pointer should be node, 
								//index can be k
								
							}
							else {
								DEBUG(dbgs() << "Not matching pattern because the non-argument was not a binary operator and not a gep\n");
								DEBUG(dbgs() << *node << "\n");
								DEBUG(dbgs() << "\t" << *node->getIncomingValue(other) << "\n");
								continue;
							}
						}
					}
				}
			}
		}
		if (TESTING) {
            for (const Function &func : module) {
                for (const BasicBlock &block : func) {
                    block.dump();
                }
            }
	    }
	    if (!testOutputName.empty()) {
            std::ofstream out(testOutputName);
            llvm::raw_os_ostream sink(out);	
            print(sink, &module);
            sink.flush();
            out.close();
        }
		return modified;
	}

	void NoPointerArithmetic::print(raw_ostream &out, const Module *module) const {
	    if (!TESTING) return;
	    for (const Function &func : *module) {
	        for (const BasicBlock &block : func) {
	            out << block;
	        }
	    }
	}
}
