#ifndef INCLUDE_BLOCK_SET_HH
#define INCLUDE_BLOCK_SET_HH

#include <unordered_set>

namespace llvm {
	class BasicBlock;
}


using BlockSet = std::unordered_set<const llvm::BasicBlock *>;


#endif	// !INCLUDE_BLOCK_SET_HH
