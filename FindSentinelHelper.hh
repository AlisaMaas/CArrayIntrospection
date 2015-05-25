#ifndef INCLUDE_FIND_SENTINEL_HELPER_HH
#define INCLUDE_FIND_SENTINEL_HELPER_HH

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/BasicBlock.h>
#include <unordered_set>
#include <unordered_map>
#include <set>


/**
* Collection of typedefs to be used in a generic manner.
**/

typedef std::unordered_set<const llvm::BasicBlock *> BlockSet;
typedef std::set<const llvm::Value *> ValueSet;
typedef std::unordered_map<const ValueSet*, std::pair<BlockSet, bool>> ValueSetToBlockSet;
typedef std::pair<BlockSet, bool> ValueReport;
typedef std::pair<llvm::BasicBlock*, std::pair<std::vector<llvm::BasicBlock*>, llvm::SmallVector<llvm::BasicBlock *, 4>>> LoopInformation;


// access to analysis results
typedef std::unordered_map<const llvm::BasicBlock *, ValueSetToBlockSet> FunctionResults;

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
ValueReport findSentinelChecks(const LoopInformation &loop, const llvm::Value * goal);

#endif // !INCLUDE_FIND_SENTINEL_HELPER_HH
