#ifndef INCLUDE_VALUE_SET_HH
#define INCLUDE_VALUE_SET_HH

#include <llvm/IR/Value.h>

#include <set>
/**
* Collection of typedefs to be used in a generic manner.
**/

typedef std::set<const llvm::Value *> ValueSet;
typedef std::set<const ValueSet *> ValueSetSet;

inline const ValueSet* getValueSetFromValue(const llvm::Value *value, ValueSetSet valueSets) {
        for (const ValueSet *valueSet : valueSets) {
            if (valueSet != nullptr) {
                for (const llvm::Value * other : *valueSet) {
                    if (other == value) return valueSet;
                }
            }
        }
        return nullptr;
}


#endif // !INCLUDE_VALUE_SET_HH