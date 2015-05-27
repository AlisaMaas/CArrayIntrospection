#define DEBUG_TYPE "no-pointer-arithmetic"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>

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

	char NoPointerArithmetic::ID;


	inline NoPointerArithmetic::NoPointerArithmetic()
		: ModulePass(ID) {
	}

	void NoPointerArithmetic::getAnalysisUsage(AnalysisUsage &) const {
	}


	bool NoPointerArithmetic::runOnModule(Module &module) {
		bool modified = false;
		LLVMContext &context = module.getContext();
		for (Function &func : module) {
			for(Argument &arg : func.getArgumentList()) {
				if (arg.getType()->isPointerTy()) {
					for (User *user : arg.users()) {
						if (PHINode *node = dyn_cast<PHINode>(user)) {
							if (node->getNumIncomingValues() != 2) {
								errs() << "Found something close to pattern with other than 2 args\n";
								errs() << *node << "\n";
								continue;
							}
							int argument = 0;
							int other = 1;
							Value *toAdd = nullptr;
							BinaryOperator *oldAdd = nullptr;
							if (node->getIncomingValue(0) != &arg) {
								argument = 1;
								other = 0;
							}
							if (BinaryOperator *binOp = dyn_cast<BinaryOperator>(node->getIncomingValue(other))) {
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
							else if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(node->getIncomingValue(other))){
								errs() << "Found a gep!\n";
								if (gep->getPointerOperand() != node) {
									errs() << "Not matching pattern because the gep didn't have the pointer in it.\n";
									errs() << *gep << "\n";
									continue;
								}
								else if (gepi->getNumIndices() != 1) {
									errs() << "Not matching pattern because there is more than one index.\n";
									errs() << *gep << "\n";
									continue;
								}
								else {
									
								}
								//pointer should be node, 
								//index can be k
								
							}
							else {
								errs() << "Not matching pattern because the non-argument was not a binary operator and not a gep\n";
								errs() << *node << "\n";
								errs() << "\t" << *node->getIncomingValue(other) << "\n";
								continue;
							}
						}
					}
				}
			}
		}
	
	
		return modified;
	}

	void NoPointerArithmetic::print(raw_ostream &, const Module *) const {
	
	}
}