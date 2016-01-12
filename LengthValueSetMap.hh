#ifndef INCLUDE_LENGTH_VALUE_SET_MAP_HH
#define INCLUDE_LENGTH_VALUE_SET_MAP_HH

#include "ValueSet.hh"

#include <map>
#include <memory>


using LengthValueSetMap = std::map<ValueSet const*, ValueSet const*>;

using SharedLengthValueSetMap = std::map<std::shared_ptr<const ValueSet>, std::shared_ptr<const ValueSet>>;


#endif	// !INCLUDE_LENGTH_VALUE_SET_MAP_HH
