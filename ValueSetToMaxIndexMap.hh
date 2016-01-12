#ifndef INCLUDE_VALUE_SET_TO_MAX_INDEX_MAP_HH
#define INCLUDE_VALUE_SET_TO_MAX_INDEX_MAP_HH

#include "ValueSet.hh"

#include <map>
#include <memory>


using ValueSetToMaxIndexMap = std::map<ValueSet const *, long int>;

using SharedValueSetToMaxIndexMap = std::map<std::shared_ptr<const ValueSet>, long int>;


#endif	// !INCLUDE_VALUE_SET_TO_MAX_INDEX_MAP_HH
