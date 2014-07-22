#include <boost/range/iterator_range.hpp>
#include "IIGlueReader.hh"
#include <llvm/Analysis/LoopPass.h>
#include <llvm/PassManager.h>
#include <llvm/Support/PatternMatch.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include "PatternMatch-extras.hh"

using namespace boost;
using namespace llvm;
using namespace llvm::PatternMatch;

/**
 * This method checks whether a given list of sentinel checks is optional using a modified depth first search.
 * The basic question this attempts to answer is: "Is there a path from loop entry to loop entry without passing
 * through a sentinel check?"
 *
 * Sentinel checks are pre-added to foundSoFar, current starts as loop entry, and the goal is loop entry. 
 * Since in some circumstances loop entry is its own successor, we must ignore the case where loop entry is the 
 * successor and parent. We simply skip that successor, if it exists.
 **/
static bool DFSCheckSentinelOptional(std::unordered_set<BasicBlock*> &foundSoFar, BasicBlock *current, BasicBlock *goal) {
	//If the current node is null, there can't be a path from it to loop entry without going through a sentinel check.
	if (current == NULL) {
		return false;
	}

	foundSoFar.insert(current);

	for (BasicBlock *succ : make_iterator_range(succ_begin(current), succ_end(current))) {
		//first make sure we haven't looked at this successor so far and it's not in the list of sentinel checks
		if (foundSoFar.find(succ) == foundSoFar.end()) {
			if (DFSCheckSentinelOptional(foundSoFar, succ, goal)) {
				return true; //if it's optional for my kids, it's optional for me.
			}		
		}
		//If the successor is the goal, first make sure it isn't its parent. If not, we found a path from loop entry to loop entry
		//while ignoring sentinel checks.
		if (succ == goal) {
			if (succ == current) {
				continue;
			}
			return true;
		}	
	}
	//If we make it all the way around, we didn't find a path from loop entry to loop entry skipping all sentinel checks.
	return false;
}

namespace
{
class FindSentinels : public FunctionPass
{
public:
	FindSentinels();
	static char ID;
	void getAnalysisUsage(AnalysisUsage &) const final;
	bool runOnFunction(Function &) override final;
};

char FindSentinels::ID;
}

static const RegisterPass<FindSentinels> registration("find-sentinels",
		"Find each branch used to exit a loop when a sentinel value is found in an array",
		true, true);


inline FindSentinels::FindSentinels()
: FunctionPass(ID)
{
}


void FindSentinels::getAnalysisUsage(AnalysisUsage &usage) const
{
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<LoopInfo>();
	usage.addRequired<IIGlueReader>();
}


bool FindSentinels::runOnFunction(Function &func) {
	LoopInfo &LI = getAnalysis<LoopInfo>();
	IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	bool foundArg = false;
	for (Function::arg_iterator args = func.arg_begin(), end = func.arg_end(); args != end; args++) {
		if (iiglue.isArray(*args)) {
			foundArg = true;
		}
	}
	if (!foundArg)
		return false;

	//We must look through all the loops to determine if any of them contain a sentinel check. 
	for (LoopInfo::iterator i = LI.begin(), e = LI.end(); i != e; ++i) {
		std::unordered_set<BasicBlock*> sentinelChecks;
		Loop *loop = *i;
		SmallVector<BasicBlock *, 4> exitingBlocks;
		loop->getExitingBlocks(exitingBlocks);
		for (const auto exitingBlock : exitingBlocks) {
			const auto terminator(exitingBlock->getTerminator());
			// to be bound to pattern elements if match succeeds      
			BasicBlock *trueBlock, *falseBlock;
			CmpInst::Predicate predicate;
			//This will need to be checked to make sure it corresponds to an argument identified as an array.
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
			// When optimized code has an OR:
			//    %arrayidx = getelementptr inbounds i8* %pointer, i64 %slot
			//    %0 = load i8* %arrayidx, align 1, !tbaa !0
			//    %cmp = icmp eq i8 %0, %goal
			//    %cmp6 = icmp eq i8 %0, 0
			//    %or.cond = or i1 %cmp, %cmp6
			//    %indvars.iv.next = add i64 %indvars.iv, 1
			//    br i1 %or.cond, label %for.end, label %for.cond
			bool matched = false;
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
			)) {
				matched = true;
			}
			//These section math a sentinel check containing an OR statement including the check for the sentinel value. 
			else if (match(terminator,
					m_Br(
							m_Or(
									m_ICmp(predicate,
											m_CombineOr(
													loadPattern,
													m_SExt(loadPattern)
											),
											m_Zero()
									),
									m_Value()),
									trueBlock,
									falseBlock)
			)) {
				matched = true;
			}
			else if (match(terminator,
					m_Br(
							m_Or(
									m_Value(),
									m_ICmp(predicate,
											m_CombineOr(
													loadPattern,
													m_SExt(loadPattern)
											),
											m_Zero()
									)),
									trueBlock,
									falseBlock)
			)) {
				matched = true;
			}
			if (matched) {
				if (!iiglue.isArray(*pointer)){
					continue;
				}		
				// check that we actually leave the loop when sentinel is found
				const BasicBlock *sentinelDestination;
				switch (predicate) {
				case CmpInst::ICMP_EQ:
					sentinelDestination = trueBlock;
					break;
				case CmpInst::ICMP_NE:
					sentinelDestination = falseBlock;
					break;
				default:
					continue;
				}
				if (loop->contains(sentinelDestination)) {
					errs() << "dest still in loop!\n";
					continue;
				}	
				// all tests pass; this is a possible sentinel check!

				errs() << "found possible sentinel check of %" << pointer->getName() << "[%" << slot->getName() << "]\n";
				errs() << "  exits loop by jumping to %" << sentinelDestination->getName() << '\n';
				//mark this block as one of the sentinel checks this loop. 
				sentinelChecks.insert(exitingBlock);
				auto induction(loop->getCanonicalInductionVariable());
				if (induction)
					errs() << "  loop has canonical induction variable %" << induction->getName() << '\n';
				else
					errs() << "  loop has no canonical induction variable\n";
			}
		}
		BasicBlock *loopEntry = *(loop->block_begin());
		if (sentinelChecks.size() == 0) {
			errs() << "There were no sentinel checks in this loop.\n";
		}
		else if (DFSCheckSentinelOptional(sentinelChecks, loopEntry, loopEntry)) {
			errs() << "The sentinel check was optional!\n";
		}
		else {
			errs() << "The sentinel check was non-optional - hooray!\n";
		}
	}

	// read-only pass never changes anything
	return false;
}
//To be used if I need passes to run at a specific time in the opt-cycle - for now, this is unnecessary since we don't actually
//run very many opt passes, just the ones from O0 and mem2reg.
/*static RegisterStandardPasses MyPassRegistration(PassManagerBuilder::EP_LoopOptimizerEnd,
  [](const PassManagerBuilder&, PassManagerBase& PM) {
    errs() << "Registered pass!\n";
    PM.add(new FindSentinels());
  });*/
