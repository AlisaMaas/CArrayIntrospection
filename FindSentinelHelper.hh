#ifndef INCLUDE_FIND_SENTINEL_HELPER_HH
#define INCLUDE_FIND_SENTINEL_HELPER_HH

#include "PatternMatchHelper.hh"

typedef std::pair<BlockSet, bool> SentinelValueReport;

/**
* Checks whether a particular loop's sentinel check is optional given the list of blocks
* previously examined. Previously examined edges are not explored again, and should include
* all sentinel checks.
**/
bool DFSCheckSentinelOptional(const llvm::Loop &loop, BlockSet &foundSoFar);
SentinelValueReport findSentinelChecks(const LoopInformation &loop, const llvm::Value * goal);

#endif // !INCLUDE_FIND_SENTINEL_HELPER_HH
