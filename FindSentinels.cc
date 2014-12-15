#include "FindSentinels.hh"
#include "IIGlueReader.hh"
#include "PatternMatch-extras.hh"

#include <boost/container/flat_set.hpp>
#include <boost/range/adaptor/indirected.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>

using namespace boost;
using namespace boost::adaptors;
using namespace llvm;
using namespace llvm::PatternMatch;
using namespace std;

static const Argument *traversePHIs(const Value *pointer, unordered_set<const PHINode*> &foundSoFar) {
	if (const Argument * const arg = dyn_cast<Argument>(pointer)) {
		return arg;
	} else if (const PHINode * const node = dyn_cast<PHINode>(pointer)) {
		if (!foundSoFar.insert(node).second) return nullptr;
		// !!! Is foundArgument always identical to (formalArg != nullptr) ?
		bool foundArgument = false;
		const Argument *formalArg = nullptr;
		for (const unsigned i : irange(0u, node->getNumIncomingValues())) {
			const Value * const v = node->getIncomingValue(i);
			if (const Argument * const arg = dyn_cast<Argument>(v)) {
				if (foundArgument) {
					// !!!: Is formalArg always null already at this point?
					formalArg = nullptr;
				} else {
					foundArgument = true;
					formalArg = arg;
				}
			} else if (isa<PHINode>(v)) {
				// !!!: We have done two dynamic checks for PHINode at this point.  It seems we should need just one.
				if (const Argument * const ret = traversePHIs(v, foundSoFar)) {
					if (foundArgument) {
						formalArg = nullptr;
					} else {
						foundArgument = true;
						formalArg = ret;
					}
				}
			}
		}
		return formalArg;
	}
	return nullptr;
}


static const Argument *traversePHIs(const Value *pointer) {
	unordered_set<const PHINode *> foundSoFar;
	return traversePHIs(pointer, foundSoFar);
}


/**
 * This mutually-recursive group of functions check whether a given list of sentinel checks is
 * optional using a modified depth first search.  The basic question they attempt to answer is: "Is
 * there a nontrivial path from loop entry to loop entry without passing through a sentinel check?"
 *
 * Sentinel checks are pre-added to foundSoFar, current starts as loop entry, and the goal is loop
 * entry.
 **/

static bool reachable(const Loop &, BlockSet &foundSoFar, const BasicBlock &current, const BasicBlock &goal);

static bool reachableNontrivially(const Loop &loop, BlockSet &foundSoFar, const BasicBlock &current, const BasicBlock &goal) {
	// look for reachable path across any one successor
	return any_of(succ_begin(&current), succ_end(&current),
			[&](const BasicBlock * const succ) { return reachable(loop, foundSoFar, *succ, goal); });
}

static bool reachable(const Loop &loop, BlockSet &foundSoFar, const BasicBlock &current, const BasicBlock &goal) {
	// mark as found so we don't revisit in the future
	const bool novel = foundSoFar.insert(&current).second;

	// already explored here, or is intentionally closed-off sentinel check
	if (!novel) return false;
	// trivially reached goal
	if (&current == &goal) {
		return true;
	}

	// not allowed to leave this loop
	if (!loop.contains(&current)) {
		return false;
	}

	// not trivially done, so look for nontrivial path
	return reachableNontrivially(loop, foundSoFar, current, goal);
}

static bool DFSCheckSentinelOptional(const Loop &loop, BlockSet &foundSoFar) {
	const BasicBlock &loopEntry = *loop.getHeader();
	return reachableNontrivially(loop, foundSoFar, loopEntry, loopEntry);
}


static const RegisterPass<FindSentinels> registration("find-sentinels",
		"Find each branch used to exit a loop when a sentinel value is found in an array",
		true, true);

char FindSentinels::ID;


inline FindSentinels::FindSentinels()
	: ModulePass(ID) {
}

void FindSentinels::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<LoopInfo>();
	usage.addRequired<IIGlueReader>();
}


bool FindSentinels::runOnModule(Module &module) {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	for (Function &func : module) {
		if ((func.isDeclaration())) continue;
		const LoopInfo &LI = getAnalysis<LoopInfo>(func);
		unordered_map<const BasicBlock *, ArgumentToBlockSet> &functionSentinelChecks = allSentinelChecks[&func];
#if 0
		// bail out early if func has no array arguments
		// up for discussion - seems to lead to some unintuitive results that I want to discuss before readding.
		if (!any_of(func.arg_begin(), func.arg_end(), [&](const Argument &arg) {
					return iiglue.isArray(arg);
				}))
			return false;
#endif
		// We must look through all the loops to determine if any of them contain a sentinel check.
		for (const Loop * const loop : LI) {
			ArgumentToBlockSet &sentinelChecks = functionSentinelChecks[loop->getHeader()];

			SmallVector<BasicBlock *, 4> exitingBlocks;
			loop->getExitingBlocks(exitingBlocks);
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
						const Argument * const formalArg = traversePHIs(pointer);

						if (formalArg == nullptr || !iiglue.isArray(*formalArg)) {
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
							DEBUG(dbgs() << "dest still in loop!\n");
							continue;
						}
						// all tests pass; this is a possible sentinel check!
						DEBUG(dbgs() << "found possible sentinel check of %" << formalArg->getName() << "[%" << slot->getName() << "]\n"
								<< "  exits loop by jumping to %" << sentinelDestination->getName() << '\n');
						// mark this block as one of the sentinel checks this loop.
						sentinelChecks[formalArg].first.insert(exitingBlock);
						auto induction(loop->getCanonicalInductionVariable());
						if (induction)
							DEBUG(dbgs() << "  loop has canonical induction variable %" << induction->getName() << '\n');
						else
							DEBUG(dbgs() << "  loop has no canonical induction variable\n");
					}
				}
				if (sentinelChecks.empty()) {
					for (const Argument &arg : iiglue.arrayArguments(func)) {
						sentinelChecks[&arg].second = true;
					}
					continue;
				}
				for (const Argument &arg : iiglue.arrayArguments(func)) {
					pair<BlockSet, bool> &checks = sentinelChecks[&arg];
					BlockSet foundSoFar = checks.first;
					checks.second = true;
					bool optional = DFSCheckSentinelOptional(*loop, foundSoFar);
					if (optional) {
						DEBUG(dbgs() << "The sentinel check was optional!\n");
						checks.second = true;
					}
					else {
						DEBUG(dbgs() << "The sentinel check was non-optional - hooray!\n");
						checks.second = false;
					}
				}
			}
	}
	// read-only pass never changes anything
	return false;
}

/**
 * Compare two BasicBlock*'s using their names.
 **/
class BasicBlockCompare { // simple comparison function
public:
	bool operator()(const BasicBlock *x, const BasicBlock *y) {
		return x->getName() < y->getName();
	}
};

/**
 * Print helper method. The output looks like the following:
 * Analyzing function: functionName
 * EITHER:	Detected no sentinel checks (end of output)
 * OR:	We found N loops.
 * FOR EACH LOOP:
 *	There are M sentinel checks in this loop(nameOfLoopHeaderBlock)
 *	We can/can not bypass all sentinel checks.
 *	Sentinel checks:
 * For each sentinel check, the name of its basic block is printed.
 **/
void FindSentinels::print(raw_ostream &sink, const Module *module) const {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	for (const Function &func : *module) {
		// print function name, how many loops found if any
		sink << "Analyzing function: " << func.getName() << '\n';
		if (allSentinelChecks.count(&func) == 0) {
			sink << "\tDetected no sentinel checks\n";
			return;
		}
		unordered_map<const BasicBlock *, ArgumentToBlockSet> loopHeaderToSentinelChecks = allSentinelChecks.at(&func);
		sink << "\tWe found: " << loopHeaderToSentinelChecks.size() << " loops\n";
		set<const BasicBlock *, BasicBlockCompare> loopHeaderBlocks;
		for (auto mapElements : loopHeaderToSentinelChecks) {
			loopHeaderBlocks.insert(mapElements.first);
		}

		// For each loop, print all sentinel checks and whether it is possible to go from loop entry to loop entry without
		// passing a sentinel check.
		for (const BasicBlock * const header : loopHeaderBlocks) {
			const ArgumentToBlockSet &entry = loopHeaderToSentinelChecks[header];
			for (const Argument &arg : iiglue.arrayArguments(func)) {
				sink << "\tExamining " << arg.getName() << " in loop " << header->getName() << '\n';
				const pair<BlockSet, bool> &checks = entry.at(&arg);
				sink << "\t\tThere are " << checks.first.size() << " sentinel checks of this argument in this loop\n";
				sink << "\t\t\tWe can" << (checks.second ? "" : "not") << " bypass all sentinel checks for this argument in this loop.\n";
				const auto names =
						checks.first
						| indirected
						| transformed([](const BasicBlock &block) {
							return block.getName().str();
						});
				// print in sorted order for consistent output
				boost::container::flat_set<string> ordered(names.begin(), names.end());
				sink << "\t\tSentinel checks: \n";
				for (const auto &sentinelCheck : ordered)
					sink << "\t\t\t" << sentinelCheck << '\n';
			}
		}
	}
}
