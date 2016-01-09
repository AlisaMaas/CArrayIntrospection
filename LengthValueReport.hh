#ifndef INCLUDE_LENGTH_VALUE_REPORT_HH
#define INCLUDE_LENGTH_VALUE_REPORT_HH

#include "BlockSet.hh"

#include <unordered_map>
#include <utility>

namespace llvm {
	class Value;
};


using LengthValueReport = std::unordered_map<const llvm::Value*, std::pair<BlockSet, bool>>;


#endif	// !INCLUDE_LENGTH_VALUE_REPORT_HH
