#define DEBUG_TYPE "find-length-loops"

#include "FindLengthLoops.hh"
#include "PatternMatch-extras.hh"
#include "ValueReachesValue.hh"

#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>


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

bool DFSCheckOptional(const LoopInformation &loop, BlockSet &foundSoFar) {
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

LengthValueReport findLengthChecks(const LoopInformation &loop, const Value * goal) {
    DEBUG(dbgs() << "Looking at " << goal->getName() << " from " << loop.first->getParent()->getName() << "\n");
	LengthValueReport lengthChecks;
	unordered_set<const Value*> slots;
	const SmallVector<BasicBlock *, 4> exitingBlocks = loop.second.second;
	for (BasicBlock *exitingBlock : exitingBlocks) {
		TerminatorInst * const terminator = exitingBlock->getTerminator();
		// to be bound to pattern elements if match succeeds
		BasicBlock *trueBlock, *falseBlock;
		CmpInst::Predicate predicate;
		// This will need to be checked to make sure it corresponds to an argument identified as an array.
		Value *pointer;
		Value *slot;
		Value *phi;
		// reusable pattern fragments
        auto addPtr = m_CombineOr(
                        m_CombineOr(
                            m_GetElementPointer(
                                m_Value(pointer),
						        m_SExt(m_Value(slot))
						    ),
						m_Load(m_Value(pointer))
					    ),
					    m_SExt(m_Load(m_Value(pointer)))
                    );
        
		if (match(terminator,
				m_Br(
					m_CombineOr(
					        m_ICmp(predicate,
					            addPtr,
					            m_Value(phi)
					        ),
					        m_ICmp(predicate,
					            m_Value(phi),
					            addPtr
					        )
					),
					trueBlock,
					falseBlock))) {
			DEBUG(dbgs() << "Matched!!!!!!\n");

			// check that we actually leave the loop when sentinel is found
			const BasicBlock *destination;
			switch (predicate) {
			case CmpInst::ICMP_EQ:
				destination = trueBlock;
				break;
			case CmpInst::ICMP_NE:
				destination = falseBlock;
				break;
			default:
				continue;
			}
			auto blocks = loop.second.first;
			if (std::find(blocks.begin(), blocks.end(), destination) != blocks.end()) {
				DEBUG(dbgs() << "dest still in loop!\n\n\n\n");
				DEBUG(dbgs() << (predicate == CmpInst::ICMP_EQ) << "\n\n\n");
				continue;
			}
			if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(pointer)) {
			    //if (dyn_cast<GetElementPtrInst>(load->getPointerOperand()))
			        if(gep->getNumIndices() == 1) 
			            pointer = gep->getPointerOperand();
			}
			/*if (LoadInst *load = dyn_cast<LoadInst>(pointer)) {
			    if (dyn_cast<GetElementPtrInst>(load->getPointerOperand()))
			        pointer = load->getPointerOperand();
			}*/
			if (!valueReachesValue(*goal, *pointer, true)) {
                DEBUG(dbgs() << "Length check indexes into incorrect value.\n");
			    continue;
			}
			if (!valueReachesValue(*goal, *phi, true)) {
			    DEBUG(dbgs() << "Length check against wrong end pointer\n");
			    continue;
			}
			// all tests pass; this is a possible sentinel check!
			DEBUG(dbgs() << "found possible length check of %" << pointer->getName() << "]\n"
				  << "  exits loop by jumping to %" << destination->getName() << '\n');
			// mark this block as one of the sentinel checks this loop.
			lengthChecks[slot].first.insert(exitingBlock);
			slots.insert(slot);
		}
	}
	for (const Value *slot : slots) {
        BlockSet foundSoFar = lengthChecks[slot].first;
        if (lengthChecks[slot].first.empty()) {
            DEBUG(dbgs() << "No length checks found for " << goal->getName() << "\n");
            lengthChecks[slot].second = true;
            return lengthChecks;
        }
        DEBUG(dbgs() << "Checking the length check for " << goal->getName() << "\n");
        bool optional = DFSCheckOptional(loop, foundSoFar);
        if (optional) {
            DEBUG(dbgs() << "The length check was optional!\n");
            lengthChecks[slot].second = true;
        }
        else {
            DEBUG(dbgs() << "The length check was non-optional - hooray!\n");
            lengthChecks[slot].second = false;
        }
        DEBUG(dbgs() << "About to return the list of length checks\n");
    }
	return lengthChecks;
}
