#define DEBUG_TYPE "no-pointer-comparisons"

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"

#include <list>
#include <map>
#include <set>

using namespace llvm;

class NoPointerComparisons : public FunctionPass {
public:
  static char ID;
  NoPointerComparisons() : FunctionPass(ID) { }

  void handleUser(Value *V, std::set<Value*> &CanGenerate);
  void handleValue(Value *V, std::set<Value*> &CanGenerate);
  void handleCmp(ICmpInst *ICI);

  bool canGenerate(Value *V, std::set<Value*> CanGenerate);

  Value *generate(Value *V);
  void replaceWithGenerated(Value *V);

  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
  virtual bool runOnFunction(Function&);

private:
  Type *Ty_;
  std::map<Value*, Argument*> Base_;
  std::map<Value*, Value*> Generated_;
};

static RegisterPass<NoPointerComparisons>
  X("no-pointer-comparisons", "Symbolic range analysis range generation test");
char NoPointerComparisons::ID = 0;

void NoPointerComparisons::handleUser(Value *V, std::set<Value*> &CanGenerate) {
  for (auto User : V->users()) {
    handleValue(User, CanGenerate);
  }
}

void NoPointerComparisons::handleValue(Value *V, std::set<Value*> &CanGenerate) {
  if (CanGenerate.count(V) > 0)
    return;

  if (isa<GetElementPtrInst>(V)
      || (isa<PHINode>(V) && V->getType()->isPointerTy())) {
    if (canGenerate(V, CanGenerate)) {
      CanGenerate.insert(V);
      handleUser(V, CanGenerate);
    }
  }
}

void NoPointerComparisons::handleCmp(ICmpInst *ICI) {
  Value *LHS = ICI->getOperand(0), *RHS = ICI->getOperand(1);
  Value *Base = Base_[LHS];
  if (!Base || Base != Base_[RHS])
    return;

  // Base == &Base[0].
  Generated_[Base] = ConstantInt::get(Ty_, 0);

  Value *LHSGen = generate(LHS);
  Value *RHSGen = generate(RHS);

  IRBuilder<> IRB(ICI);
  ICI->replaceAllUsesWith(
      IRB.CreateICmp(
          ICI->getPredicate(), LHSGen, RHSGen, ICI->getName() + ".toint"));

  replaceWithGenerated(LHS);
  replaceWithGenerated(RHS);
}

bool NoPointerComparisons::canGenerate(Value *V, std::set<Value*> CanGenerate) {
  if (CanGenerate.count(V) > 0) {
    return true;
  }

  if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V)) {
    // Assume we can generate the GEP.
    CanGenerate.insert(GEP);
    return canGenerate(GEP->getPointerOperand(), CanGenerate);
  } else if (PHINode *Phi = dyn_cast<PHINode>(V)) {
    // Assume we can generate the phi.
    CanGenerate.insert(Phi);
    for (auto &Op : Phi->operands()) {
      if (!canGenerate(Op, CanGenerate)) {
        return false;
      }
    }
    return true;
  }

  return false;
}

Value *NoPointerComparisons::generate(Value *V) {
  auto It = Generated_.find(V);
  if (It != Generated_.end()) {
    return It->second;
  }

  if (GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(V)) {
    Value *PointerGen = generate(GEP->getPointerOperand());
    IRBuilder<> IRB(GEP);
    Value *Index = GEP->getOperand(1);
    Value *IndexGen = IRB.CreateSExt(Index, Ty_, Index->getName() + ".toint");
    Value *Gen = PointerGen == GEP->getPointerOperand()
        ? IndexGen
        : IRB.CreateAdd(PointerGen, IndexGen, GEP->getName() + ".toint");
    Generated_[GEP] = Gen;
    return Gen;
  } else if (PHINode *Phi = dyn_cast<PHINode>(V)) {
    IRBuilder<> IRB(Phi);
    PHINode *Gen =
        IRB.CreatePHI(
            Ty_, Phi->getNumIncomingValues(), Phi->getName() + ".toint");
    Generated_[Phi] = Gen;
    for (unsigned Idx = 0; Idx < Phi->getNumIncomingValues(); ++Idx) {
      Value *Incoming = generate(Phi->getIncomingValue(Idx));
      Gen->addIncoming(Incoming, Phi->getIncomingBlock(Idx));
    }
    return Gen;
  }

  llvm_unreachable("Unable to generate index for value");
}

void NoPointerComparisons::replaceWithGenerated(Value *V) {
  auto BaseIt = Base_.find(V);
  auto GenIt = Generated_.find(V);
  if (BaseIt == Base_.end() || GenIt == Generated_.end())
    return;

  std::list<Value*> Users;
  for (auto User : V->users()) {
    Users.push_back(User);
  }

  Value *Base = BaseIt->second;
  Value *Index = GenIt->second;
  for (auto &User : Users) {
    if (Instruction *I = dyn_cast<Instruction>(User)) {
      IRBuilder<> IRB(I);

      // Careful about inserting the GEP near phi nodes, and always try to
      // insert the GEP as close as possible to the use.
      if (isa<PHINode>(I)) {
         if (Instruction *VI = dyn_cast<Instruction>(V)) {
           if (isa<PHINode>(VI)) {
             IRB.SetInsertPoint(VI->getParent()->getFirstNonPHI());
           } else {
             IRB.SetInsertPoint(VI);
           }
         } else if (Argument *A = dyn_cast<Argument>(V)) {
           IRB.SetInsertPoint(A->getParent()->getEntryBlock().begin());
         } else {
           IRB.SetInsertPoint(I->getParent()->getFirstNonPHI());
         }
      }

      Value *GEP =
          IRB.CreateGEP(Base, ArrayRef<Value*>(Index), I->getName() + ".gen");
      I->replaceUsesOfWith(V, GEP);
    }
  }
}

void NoPointerComparisons::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll(); // Not really, though.
}

bool NoPointerComparisons::runOnFunction(Function& F) {
  LLVMContext &C = F.getContext();
  DataLayout DL(F.getParent());

  Ty_ = IntegerType::get(C, DL.getPointerSizeInBits());
  Base_.clear();
  Generated_.clear();

  for (auto &AI : F.args()) {
    if (AI.getType()->isPointerTy()) {
      std::set<Value*> CanGenerate = {&AI};
      handleUser(&AI, CanGenerate);
      for (auto &V : CanGenerate) {
        Base_[V] = &AI;
      }
    }
  }

  for (auto &BB : F) {
    TerminatorInst *TI = BB.getTerminator();
    if (BranchInst *BI = dyn_cast<BranchInst>(TI)) {
      if (BI->isConditional()) {
        if (ICmpInst *ICI = dyn_cast<ICmpInst>(BI->getCondition())) {
          handleCmp(ICI);
        }
      }
    }
  }

  return Generated_.size() > 0;
}
