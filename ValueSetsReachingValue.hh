#ifndef INCLUDE_ARGUMENTS_REACHING_VALUE_HH
#define INCLUDE_ARGUMENTS_REACHING_VALUE_HH

#include "ValueSetSet.hh"

#include <memory>


ValueSetSet valueSetsReachingValue(const llvm::Value &, const ValueSetSet &);


#endif	// !INCLUDE_ARGUMENTS_REACHING_VALUE_HH
