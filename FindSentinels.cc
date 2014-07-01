#include <llvm/Analysis/LoopPass.h>
#include <llvm/Support/PatternMatch.h>
#include <llvm/Support/raw_ostream.h>

#include "PatternMatch-extras.hh"

using namespace llvm;
using namespace llvm::PatternMatch;


namespace
{
class FindSentinels : public LoopPass
{
public:
  FindSentinels();
  static char ID;
  void getAnalysisUsage(AnalysisUsage &) const final;
  bool runOnLoop(Loop *, LPPassManager &) override final;
};

  char FindSentinels::ID;
}

static const RegisterPass<FindSentinels> registration("find-sentinels",
						      "Find each branch used to exit a loop when a sentinel value is found in an array",
						      true, true);


inline FindSentinels::FindSentinels()
  : LoopPass(ID)
{
}


void FindSentinels::getAnalysisUsage(AnalysisUsage &usage) const
{
  // read-only pass never changes anything
  usage.setPreservesAll();
}


bool FindSentinels::runOnLoop(Loop *loop, LPPassManager &)
{
  SmallVector<BasicBlock *, 4> exitingBlocks;
  loop->getExitingBlocks(exitingBlocks);

  for (const auto exitingBlock : exitingBlocks)
    {
      const auto terminator(exitingBlock->getTerminator());

      // to be bound to pattern elements if match succeeds      
      BasicBlock *trueBlock, *falseBlock;
      CmpInst::Predicate predicate;
      Argument *pointer;
      Value *slot;

      // reusable pattern fragment
      auto loadPattern(m_Load(
			      m_GetElementPointer(
						  m_FormalArgument(pointer),
						  m_Value(slot)
						  )
			      ));

      // Clang 3.4 without optimization, after running mem2reg:
      //
      //     %0 = getelementptr inbounds i8* %pointer, i64 %slot
      //     %1 = load i8* %0, align 1
      //     %element = sext i8 %1 to i32
      //     %2 = icmp ne i32 %element, 0
      //     br i1 %2, label %trueBlock, label %falseBlock
      //
      // Clang 3.4 with any level of optimization:
      //
      //     %0 = getelementptr inbounds i8* %pointer, i64 %slot
      //     %1 = load i8* %0, align 1
      //     %element = icmp eq i8 %1, 0
      //     br i1 %element, label %trueBlock, label %falseBlock

      if (match(terminator,
		m_Br(
		     m_ICmp(predicate,
			    m_CombineOr(
					loadPattern,
					m_SExt(loadPattern)
					),
			    m_Zero()
			    ),
		     trueBlock,
		     falseBlock)
		))
	{
	  // check that we actually leave the loop when sentinel is found
	  const BasicBlock *sentinelDestination;
	  switch (predicate)
	    {
	    case CmpInst::ICMP_EQ:
	      sentinelDestination = trueBlock;
	      break;
	    case CmpInst::ICMP_NE:
	      sentinelDestination = falseBlock;
	      break;
	    default:
	      continue;
	    }
	  if (loop->contains(sentinelDestination)) continue;
	  
	  // all tests pass; this is a possible sentinel check!

	  errs() << "found possible sentinel check of %" << pointer->getName() << "[%" << slot->getName() << "]\n";
	  errs() << "  exits loop by jumping to %" << sentinelDestination->getName() << '\n';

	  auto induction(loop->getCanonicalInductionVariable());
	  if (induction)
	    errs() << "  loop has canonical induction variable %" << induction->getName() << '\n';
	  else
	    errs() << "  loop has no canonical induction variable\n";
	}
    }

  // read-only pass never changes anything
  return false;
}
