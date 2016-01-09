#ifndef INCLUDE_ANNOTATION_MAP_HH
#define INCLUDE_ANNOTATION_MAP_HH

#include "ValueSet.hh"

#include <unordered_map>

class LengthInfo;


using AnnotationMap = std::unordered_map<const ValueSet *, LengthInfo>;


#endif // !INCLUDE_ANNOTATION_MAP_HH
