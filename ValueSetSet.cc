#include "ValueSetSet.hh"


const ValueSet* ValueSetSet::getValueSetFromValue(const llvm::Value *value) const {
        for (const ValueSet *valueSet : *this) {
		if (valueSet != nullptr) {
			for (const llvm::Value *other : *valueSet) {
				if (other == value) return valueSet;
			}
		}
        }
        return nullptr;
}
