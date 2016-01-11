#include "ValueSetSet.hh"


template <>
const ValueSet* ValueSetSet<const ValueSet *>::getValueSetFromValue(const llvm::Value *value) const {
        for (const ValueSet *valueSet : *this)
		if (valueSet != nullptr && valueSet->count(value))
			return valueSet;

        return nullptr;
}


template <>
const ValueSet* ValueSetSet<ValueSet>::getValueSetFromValue(const llvm::Value *value) const {
        for (const ValueSet &valueSet : *this)
		if (valueSet.count(value))
			return &valueSet;

        return nullptr;
}


std::shared_ptr<ValueSet> ValueSetSet<std::shared_ptr<ValueSet>>::getValueSetFromValue(const llvm::Value *value) const {
        for (const auto &valueSet : *this)
		if (valueSet && valueSet->count(value))
			return valueSet;

        return { };
}
