#ifndef INCLUDE_ARGUMENTS_REACHING_VALUE_HH
#define INCLUDE_ARGUMENTS_REACHING_VALUE_HH

#include "ValueSetSet.hh"



template <typename VS>
ValueSetSet<const ValueSet *> valueSetsReachingValue(const llvm::Value &, const ValueSetSet<VS> &);



#endif	// !INCLUDE_ARGUMENTS_REACHING_VALUE_HH
