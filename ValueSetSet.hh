#ifndef INCLUDE_VALUE_SET_SET_HH
#define INCLUDE_VALUE_SET_SET_HH

#include "ValueSet.hh"

namespace llvm  {
	class Value;
}


struct ValueSetSet : public std::set<const ValueSet *> {
	const ValueSet *getValueSetFromValue(const llvm::Value *) const;
};


#endif // !INCLUDE_VALUE_SET_SET_HH
