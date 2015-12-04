#include "ValueSetSet.hh"


const ValueSet* ValueSetSet::getValueSetFromValue(const llvm::Value *value) const {
        for (const ValueSet *valueSet : *this)
		if (valueSet != nullptr && valueSet->count(value))
			return valueSet;

        return nullptr;
}
