#include "ValueSetSet.hh"


std::shared_ptr<const ValueSet> ValueSetSet::getValueSetFromValue(const llvm::Value *value) const {
        for (const auto &valueSet : *this)
		if (valueSet && valueSet->count(value))
			return valueSet;

        return { };
}
