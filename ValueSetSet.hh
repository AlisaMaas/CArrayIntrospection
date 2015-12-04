#ifndef INCLUDE_VALUE_SET_SET_HH
#define INCLUDE_VALUE_SET_SET_HH

#include "ValueSet.hh"

namespace llvm  {
	class Value;
}


using ValueSetSet = std::set<const ValueSet *>;


const ValueSet *getValueSetFromValue(const llvm::Value *, const ValueSetSet &);


#endif // !INCLUDE_VALUE_SET_SET_HH
