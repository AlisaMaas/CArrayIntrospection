#include "ValueSetSet.hh"


const ValueSet* getValueSetFromValue(const llvm::Value *value, const ValueSetSet &valueSets) {
        for (const ValueSet *valueSet : valueSets) {
            if (valueSet != nullptr) {
                for (const llvm::Value * other : *valueSet) {
                    if (other == value) return valueSet;
                }
            }
        }
        return nullptr;
}
