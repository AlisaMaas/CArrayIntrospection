#ifndef INCLUDE_VALUE_SET_SET_HH
#define INCLUDE_VALUE_SET_SET_HH

#include <memory>
#include "ValueSet.hh"

namespace llvm  {
	class Value;
}


struct ValueSetSet : public std::set<std::shared_ptr<const ValueSet>> {
	value_type getValueSetFromValue(const llvm::Value *) const;
};


#endif // !INCLUDE_VALUE_SET_SET_HH
