#ifndef INCLUDE_CALL_INST_SET_HH
#define INCLUDE_CALL_INST_SET_HH

#include <unordered_set>

namespace llvm {
	class CallInst;
}


using CallInstSet = std::unordered_set<const llvm::CallInst *>;


#endif	// !INCLUDE_CALL_INST_SET_HH
