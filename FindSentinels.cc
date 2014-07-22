#include "IIGlueReader.hh"
#include "PatternMatch-extras.hh"

#include <llvm/Analysis/LoopPass.h>
#include <llvm/PassManager.h>
#include <llvm/Support/PatternMatch.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>

using namespace llvm;
using namespace llvm::PatternMatch;

/**
 * This mutually-recursive group of functions check whether a given list of sentinel checks is
 * optional using a modified depth first search.  The basic question they attempt to answer is: "Is
 * there a nontrivial path from loop entry to loop entry without passing through a sentinel check?"
 *
 * Sentinel checks are pre-added to foundSoFar, current starts as loop entry, and the goal is loop
 * entry.
 **/

typedef std::unordered_set<const BasicBlock *> BlockSet;

static bool reachable(const Loop &, BlockSet &foundSoFar, const BasicBlock &current, const BasicBlock &goal);

static bool reachableNontrivially(const Loop &loop, BlockSet &foundSoFar, const BasicBlock &current, const BasicBlock &goal) {
	// look for reachable path across any one successor
	return std::any_of(succ_begin(&current), succ_end(&current),
			   [&](const BasicBlock * const succ) { return reachable(loop, foundSoFar, *succ, goal); });
}

static bool reachable(const Loop &loop, BlockSet &foundSoFar, const BasicBlock &current, const BasicBlock &goal) {

	// trivially reached goal
	if (&current == &goal) return true;

	// not allowed to leave this loop
	if (!loop.contains(&current)) return false;

	// mark as found so we don't revisit in the future
	const bool novel = foundSoFar.insert(&current).second;

	// already explored here, or is intentionally closed-off sentinel check
	if (!novel) return false;

	// not trivially done, so look for nontrivial path
	return reachableNontrivially(loop, foundSoFar, current, goal);
}

static bool DFSCheckSentinelOptional(const Loop &loop, BlockSet &foundSoFar) {
	const BasicBlock &loopEntry = *loop.getHeader();
	return reachableNontrivially(loop, foundSoFar, loopEntry, loopEntry);
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
		BlockSet sentinelChecks;
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
		if (sentinelChecks.size() == 0) {
			errs() << "There were no sentinel checks in this loop.\n";
		}
		else if (DFSCheckSentinelOptional(*loop, sentinelChecks)) {
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
