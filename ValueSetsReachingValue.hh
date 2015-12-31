#ifndef INCLUDE_ARGUMENTS_REACHING_VALUE_HH
#define INCLUDE_ARGUMENTS_REACHING_VALUE_HH

#include "BacktrackPhiNodes.hh"
#include "ValueSetSet.hh"

#include <llvm/Support/Casting.h>
#include <set>


////////////////////////////////////////////////////////////////////////
//
//  collect the set of all arguments that may flow to a given value
//  across zero or more phi nodes
//

class ValueSetsReachingValue : public BacktrackPhiNodes {
public:
	ValueSetsReachingValue(const ValueSetSet &values);
	void visit(const llvm::Value &) final override;
	ValueSetSet result;
	static ValueSetSet valueSetsReachingValue(const llvm::Value &start, const std::set<const ValueSet*> values);
private:
	const ValueSetSet &valueSets;
};


inline ValueSetsReachingValue::ValueSetsReachingValue(const ValueSetSet &values)
	: valueSets(values) {
}


inline void ValueSetsReachingValue::visit(const llvm::Value &reached) {
	const ValueSet* valueSet = valueSets.getValueSetFromValue(&reached);
	if (valueSet)
		result.insert(valueSet);
}

inline static ValueSetSet valueSetsReachingValue(const llvm::Value &start, const ValueSetSet &values) {
	ValueSetsReachingValue explorer(values);
	explorer.backtrack(start);
	return std::move(explorer.result);
}

#endif	// !INCLUDE_ARGUMENTS_REACHING_VALUE_HH
