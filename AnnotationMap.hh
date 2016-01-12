#ifndef INCLUDE_ANNOTATION_MAP_HH
#define INCLUDE_ANNOTATION_MAP_HH

#include "ValueSet.hh"

#include <memory>
#include <unordered_map>

class LengthInfo;


using AnnotationMap = std::unordered_map<std::shared_ptr<const ValueSet>, LengthInfo>;


#endif // !INCLUDE_ANNOTATION_MAP_HH
