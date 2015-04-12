#ifndef INCLUDE_FIND_SENTINEL_HELPER_HH
#define INCLUDE_FIND_SENTINEL_HELPER_HH

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/BasicBlock.h>
#include <unordered_set>
#include <unordered_map>


/**
* Collection of typedefs to be used in a generic manner.
**/

typedef std::unordered_set<const llvm::BasicBlock *> BlockSet;
typedef std::unordered_map<const llvm::Value *, std::pair<BlockSet, bool>> ValueToBlockSet;
// access to analysis results derived by this pass
typedef std::unordered_map<const llvm::BasicBlock *, ValueToBlockSet> FunctionResults;

/**
* Collection of functions that can be abstracted out to find sentinel checks of a specific
* value, not necessarily an argument.
**/

/**
* Checks whether a particular loop's sentinel check is optional given the list of blocks
* previously examined. Previously examined edges are not explored again, and should include
* all sentinel checks.
**/
bool DFSCheckSentinelOptional(const llvm::Loop &loop, BlockSet &foundSoFar);
ValueToBlockSet findSentinelChecks(const llvm::Loop * const loop);

#endif // !INCLUDE_FIND_SENTINEL_HELPER_HH
