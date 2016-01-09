#ifndef PATTERN_MATCH_HELPER
#define PATTERN_MATCH_HELPER

#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/BasicBlock.h>
#include <unordered_map>
#include <unordered_set>

#include "BlockSet.hh"
#include "ValueSet.hh"

/**
* Collection of typedefs to be used in a generic manner.
**/

typedef std::unordered_map<const ValueSet*, std::pair<BlockSet, bool>> ValueSetToBlockSet;
typedef std::pair<llvm::BasicBlock*, std::pair<std::vector<llvm::BasicBlock*>, llvm::SmallVector<llvm::BasicBlock *, 4>>> LoopInformation;



#endif // !PATTERN_MATCH_HELPER
