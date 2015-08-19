#ifndef INCLUDE_FIND_LENGTH_LOOPS_HELPER_HH
#define INCLUDE_FIND_LENGTH_LOOPS_HELPER_HH

#include "PatternMatchHelper.hh"

typedef std::unordered_map<const llvm::Value*, std::pair<BlockSet, bool>> LengthValueReport;

/**
* Checks whether a particular loop's sentinel check is optional given the list of blocks
* previously examined. Previously examined edges are not explored again, and should include
* all sentinel checks.
**/
bool DFSCheckOptional(const llvm::Loop &loop, BlockSet &foundSoFar);
LengthValueReport findLengthChecks(const LoopInformation &loop, const llvm::Value * goal);

#endif // !INCLUDE_FIND_LENGTH_LOOPS_HELPER_HH
