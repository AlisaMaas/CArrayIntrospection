#ifndef INCLUDE_VALUE_SET_SET_HH
#define INCLUDE_VALUE_SET_SET_HH

#include "ValueSet.hh"

namespace llvm  {
	class Value;
}


template <class VS = const ValueSet *>
struct ValueSetSet : public std::set<VS> {
	const ValueSet *getValueSetFromValue(const llvm::Value *) const;
};


#endif // !INCLUDE_VALUE_SET_SET_HH
