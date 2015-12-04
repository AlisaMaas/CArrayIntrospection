#ifndef INCLUDE_VALUE_SET_HH
#define INCLUDE_VALUE_SET_HH

#include <set>

namespace llvm {
	class Value;
}


typedef std::set<const llvm::Value *> ValueSet;


#endif // !INCLUDE_VALUE_SET_HH
