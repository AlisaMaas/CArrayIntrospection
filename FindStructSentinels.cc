#define DEBUG_TYPE "find-struct-sentinels" 
#include "BacktrackPhiNodes.hh"
#include "ElementsReachingValue.hh"
#include "FindStructElements.hh"
#include "FindStructSentinels.hh"
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

static const RegisterPass<FindStructSentinels> registration("find-struct-sentinels",
        "Find each branch used to exit a loop when a sentinel value is found in an array",
        true, true);

char FindStructSentinels::ID;


inline FindStructSentinels::FindStructSentinels()
    : ModulePass(ID) {
}

void FindStructSentinels::getAnalysisUsage(AnalysisUsage &usage) const {
    // read-only pass never changes anything
    usage.setPreservesAll();
    usage.addRequired<LoopInfo>();
    usage.addRequired<FindStructElements>();
}


bool FindStructSentinels::runOnModule(Module &module) {
    FindStructElements &findStructElements = getAnalysis<FindStructElements>();
    elementToValueSet = findStructElements.getStructElements();
    for (Function &func : module) {
        if ((func.isDeclaration())) continue;
        DEBUG(dbgs() << "Analyzing " << func.getName() << "\n");
        const LoopInfo &LI = getAnalysis<LoopInfo>(func);
        FunctionResults &functionSentinelChecks = allSentinelChecks[&func];
        std::set<StructElement> matched;
        for (const Loop * const loop : LI) {
            const ValueToBlockSet &foundSentinelChecks = findSentinelChecks(loop);
            ValueSetToBlockSet &sentinelChecks = functionSentinelChecks[loop->getHeader()];
            for (auto pair : foundSentinelChecks) {
                const ElementSet reaching = elementsReachingValue(*pair.first);
                if (reaching.empty()) continue;

                // Two or more is possible,
                // but we don't handle it yet.
                //TODO: multiple reaching is suddenly more of a possibility - handle this in the
                //near future.
                assert(reaching.size() == 1);
                const StructElement &element = **reaching.begin();
                assert(elementToValueSet.count(element));
                if (!sentinelChecks.count(elementToValueSet[element])) { 
                    sentinelChecks[elementToValueSet[element]] = pair.second;
                }
                else {
                    sentinelChecks[elementToValueSet[element]].first.insert(pair.second.first.begin(), pair.second.first.end());
                    sentinelChecks[elementToValueSet[element]].second &= pair.second.second;
                }
                matched.insert(element);
                errs() << "Reporting that " << element.first->getName() << " at " <<  element.second << " is " << pair.second.second << "\n";
            }
            DEBUG(dbgs() << "About to add all the missing elements to the map\n");
            for (auto &tuple : elementToValueSet) {
                if (!matched.count(tuple.first)) {
                    DEBUG(dbgs() << "Adding missing element " << tuple.first.first->getName() << "[" << tuple.first.second << "]\n");
                    assert(elementToValueSet[tuple.first]);
                    sentinelChecks[tuple.second].second = true;
                }
            }
            DEBUG(dbgs() << "Done with the missing elements\n");
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
 * EITHER:    Detected no sentinel checks (end of output)
 * OR:    We found N loops.
 * FOR EACH LOOP:
 *    There are M sentinel checks in this loop(nameOfLoopHeaderBlock)
 *    We can/can not bypass all sentinel checks.
 *    Sentinel checks:
 * For each sentinel check, the name of its basic block is printed.
 **/
void FindStructSentinels::print(raw_ostream &sink, const Module *module) const {
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
            for (const auto &elementAndValueSet : elementToValueSet) {
                for (auto tuple : entry) {
                    if (tuple.first == elementAndValueSet.second) {
                        const pair<BlockSet, bool> &checks = tuple.second;
                        if (checks.first.empty()) continue;
                        sink << "\tExamining " << str(&elementAndValueSet.first) << " in loop " << header.getName() << '\n';
                        sink << "\t\tThere are " << checks.first.size() << " sentinel checks of this element in this loop\n";
                        sink << "\t\t\tWe can" << (checks.second ? "" : "not") << " bypass all sentinel checks for this element in this loop.\n";
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
