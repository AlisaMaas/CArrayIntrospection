#ifndef INCLUDE_ARGUMENTS_REACHING_VALUE_HH
#define INCLUDE_ARGUMENTS_REACHING_VALUE_HH

#include "ValueSetSet.hh"

#include <memory>


template <typename VS>
ValueSetSet<const ValueSet *> valueSetsReachingValue(const llvm::Value &, const ValueSetSet<VS> &);

ValueSetSet<std::shared_ptr<const ValueSet>> valueSetsReachingValue(const llvm::Value &, const ValueSetSet<std::shared_ptr<const ValueSet>> &);


#endif	// !INCLUDE_ARGUMENTS_REACHING_VALUE_HH
