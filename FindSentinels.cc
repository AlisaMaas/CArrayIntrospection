#define DEBUG_TYPE "find-sentinels" 
#include "BacktrackPhiNodes.hh"
#include "ArgumentsReachingValue.hh"
#include "FindSentinels.hh"
#include "IIGlueReader.hh"
#include "PatternMatch-extras.hh"

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/range/adaptor/indirected.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/irange.hpp>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>

using namespace boost;
using namespace boost::adaptors;
using namespace boost::container;
using namespace llvm;
using namespace llvm::PatternMatch;
using namespace std;

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
		DEBUG(dbgs() << "Analyzing " << func.getName() << "\n");
		const LoopInfo &LI = getAnalysis<LoopInfo>(func);
		FunctionResults &functionSentinelChecks = allSentinelChecks[&func];
#if 0
		// bail out early if func has no array arguments
		// up for discussion - seems to lead to some unintuitive results that I want to discuss before readding.
		if (!any_of(func.arg_begin(), func.arg_end(), [&](const Argument &arg) {
					return iiglue.isArray(arg);
				}))
			return false;
#endif
		ValueSet matched;
		for (const Loop * const loop : LI) {
			const ValueToBlockSet &foundSentinelChecks = findSentinelChecks(loop);
			ValueSetToBlockSet &sentinelChecks = functionSentinelChecks[loop->getHeader()];
			for (auto pair : foundSentinelChecks) {
				const ArgumentSet reaching = argumentsReachingValue(*pair.first);
				if (reaching.empty()) continue;

				// Two or more is possible,
				// but we don't handle it yet.
				assert(reaching.size() == 1);
				const Argument &formalArg = **reaching.begin();
				if (!iiglue.isArray(formalArg)) continue;
				if (!argumentToValueSet.count(&formalArg)) {
					ValueSet *values = new unordered_set<const Value*>();
					values->insert(&formalArg);
					sentinelChecks[values] = pair.second;
					argumentToValueSet[&formalArg] = values;
					matched.insert(&formalArg);
				}
				else {
					sentinelChecks[argumentToValueSet[&formalArg]] = pair.second;
					matched.insert(&formalArg);
				}
				DEBUG(dbgs() << "Reporting that " << formalArg.getName() << " is " << pair.second.second << "\n");
			}
			DEBUG(dbgs() << "About to add all the missing arguments to the map\n");
			for (const Argument &arg : iiglue.arrayArguments(func)) {
				if (!matched.count(&arg)) {
					DEBUG(dbgs() << "Adding missing argument " << arg.getName() << "\n");
					ValueSet *values = new unordered_set<const Value*>();
					values->insert(&arg);
					sentinelChecks[values].second = true;
					argumentToValueSet[&arg] = values;
				}
			}
			DEBUG(dbgs() << "Done with the missing arguments\n");
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
	bool operator()(const BasicBlock *x, const BasicBlock *y) const {
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
		const FunctionResults &unorderedChecks = allSentinelChecks.at(&func);
		sink << "\tWe found: " << unorderedChecks.size() << " loops\n";

		// For each loop, print all sentinel checks and whether it is possible to go from loop entry to loop entry without
		// passing a sentinel check.
		const flat_map<const BasicBlock *, ValueSetToBlockSet, BasicBlockCompare> orderedChecks(unorderedChecks.begin(), unorderedChecks.end());
		for (const auto &check : orderedChecks) {
			const BasicBlock &header = *check.first;
			const ValueSetToBlockSet &entry = check.second;
			for (const Argument &arg : iiglue.arrayArguments(func)) {
				for (auto tuple : entry) {
					if (tuple.first->count(&arg)) {
						const pair<BlockSet, bool> &checks = tuple.second;
						if (checks.first.empty()) continue;
						sink << "\tExamining " << arg.getName() << " in loop " << header.getName() << '\n';
						sink << "\t\tThere are " << checks.first.size() << " sentinel checks of this argument in this loop\n";
						sink << "\t\t\tWe can" << (checks.second ? "" : "not") << " bypass all sentinel checks for this argument in this loop.\n";
						const auto names =
								checks.first
								| indirected
								| transformed([](const BasicBlock &block) {
									return block.getName().str();
								});
						// print in sorted order for consistent output
						flat_set<string> ordered(names.begin(), names.end());
						sink << "\t\tSentinel checks: \n";
						for (const auto &sentinelCheck : ordered)
							sink << "\t\t\t" << sentinelCheck << '\n';
						break;
					}
				}
			}
		}
	}
}
