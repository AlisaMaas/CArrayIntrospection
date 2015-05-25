#define DEBUG_TYPE "find-sentinels-helper"

#include "FindSentinelHelper.hh"
#include "PatternMatch-extras.hh"
#include "ValueReachesValue.hh"

#include <llvm/Support/Debug.h>


using namespace llvm;
using namespace llvm::PatternMatch;
using namespace std;

/**
 * This mutually-recursive group of functions check whether a given list of sentinel checks is
 * optional using a modified depth first search.  The basic question they attempt to answer is: "Is
 * there a nontrivial path from loop entry to loop entry without passing through a sentinel check?"
 *
 * Sentinel checks are pre-added to foundSoFar, current starts as loop entry, and the goal is loop
 * entry.
 **/

static bool reachable(const LoopInformation &, BlockSet &foundSoFar, const BasicBlock &current, const BasicBlock &goal);

static bool reachableNontrivially(const LoopInformation &loop, BlockSet &foundSoFar, const BasicBlock &current, const BasicBlock &goal) {
	// look for reachable path across any one successor
	return any_of(succ_begin(&current), succ_end(&current),
			[&](const BasicBlock * const succ) { return reachable(loop, foundSoFar, *succ, goal); });
}

static bool reachable(const LoopInformation &loop, BlockSet &foundSoFar, const BasicBlock &current, const BasicBlock &goal) {
	// mark as found so we don't revisit in the future
	const bool novel = foundSoFar.insert(&current).second;

	// already explored here, or is intentionally closed-off sentinel check
	if (!novel) { 
	    return false;
	}
	// trivially reached goal
	if (&current == &goal) {
		return true;
	}

	// not allowed to leave this loop
	auto blocks = loop.second.first;
	if (std::find(blocks.begin(), blocks.end(), &current) == blocks.end()) {
		return false;
	}

	// not trivially done, so look for nontrivial path
	return reachableNontrivially(loop, foundSoFar, current, goal);
}

bool DFSCheckSentinelOptional(const LoopInformation &loop, BlockSet &foundSoFar) {
	const BasicBlock &loopEntry = *loop.first;
	return reachableNontrivially(loop, foundSoFar, loopEntry, loopEntry);
}

/*static Value* stripLoadsAndStores(Value *value) {
    
}*/

/**
* Find all sentinel checks for goal in loop,
* and determine whether they are together 
* optional or non-optional.
*
* @return A pair of set of blocks, bool corresponding
* to the sentinel checks and their optionality.
**/

ValueReport findSentinelChecks(const LoopInformation &loop, const Value * goal) {
    DEBUG(dbgs() << "Looking at " << goal->getName() << " from " << loop.first->getParent()->getName() << "\n");
	ValueReport sentinelChecks;
	const SmallVector<BasicBlock *, 4> exitingBlocks = loop.second.second;
	for (BasicBlock *exitingBlock : exitingBlocks) {
		TerminatorInst * const terminator = exitingBlock->getTerminator();
		// to be bound to pattern elements if match succeeds
		BasicBlock *trueBlock, *falseBlock;
		CmpInst::Predicate predicate;
		// This will need to be checked to make sure it corresponds to an argument identified as an array.
		Value *pointer;
		Value *slot;

		// reusable pattern fragments

		auto loadPattern = m_Load(
				m_GetElementPointer(
						m_Value(pointer),
						m_Value(slot)
						)
				);

		auto compareZeroPattern = m_ICmp(predicate,
				m_CombineOr(
						loadPattern,
						m_SExt(loadPattern)
						),
						m_Zero()
				);

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
		if (match(terminator,
				m_Br(
					m_CombineOr(
							compareZeroPattern,
							m_CombineOr(
								m_Or(
									compareZeroPattern,
									m_Value()
									),
								m_Or(
									m_Value(),
									compareZeroPattern
									)
							)
					),
					trueBlock,
					falseBlock))) {
			DEBUG(dbgs() << "Matched!!!!!!\n");

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
			auto blocks = loop.second.first;
			if (std::find(blocks.begin(), blocks.end(), sentinelDestination) != blocks.end()) {
				DEBUG(dbgs() << "dest still in loop!\n\n\n\n");
				DEBUG(dbgs() << (predicate == CmpInst::ICMP_EQ) << "\n\n\n");
				continue;
			}
			if (LoadInst *load = dyn_cast<LoadInst>(pointer)) {
			    if (dyn_cast<GetElementPtrInst>(load->getPointerOperand()))
			        pointer = load->getPointerOperand();
			}
			if (!valueReachesValue(*goal, *pointer, true)) {
                DEBUG(dbgs() << "Sentinel check of incorrect value.\n");
			    continue;
			}
			// all tests pass; this is a possible sentinel check!
			DEBUG(dbgs() << "found possible sentinel check of %" << pointer->getName() << "[%" << slot->getName() << "]\n"
				  << "  exits loop by jumping to %" << sentinelDestination->getName() << '\n');
			// mark this block as one of the sentinel checks this loop.
			sentinelChecks.first.insert(exitingBlock);
		}
	}
    BlockSet foundSoFar = sentinelChecks.first;
    if (sentinelChecks.first.empty()) {
        DEBUG(dbgs() << "No sentinel checks found for " << goal->getName() << "\n");
        sentinelChecks.second = true;
        return sentinelChecks;
    }
    DEBUG(dbgs() << "Checking the sentinel check for " << goal->getName() << "\n");
    bool optional = DFSCheckSentinelOptional(loop, foundSoFar);
    if (optional) {
        DEBUG(dbgs() << "The sentinel check was optional!\n");
        sentinelChecks.second = true;
    }
    else {
        DEBUG(dbgs() << "The sentinel check was non-optional - hooray!\n");
        sentinelChecks.second = false;
    }
	DEBUG(dbgs() << "About to return the list of sentinel checks\n");
	return sentinelChecks;
}
