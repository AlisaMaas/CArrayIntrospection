#include "ValueSetsReachingValue.hh"

using namespace llvm;


template <typename VS>
void ValueSetsReachingValue<VS>::visit(const Value &reached) {
	const ValueSet* valueSet = valueSets.getValueSetFromValue(&reached);
	if (valueSet)
		result.insert(valueSet);
}


template <typename VS>
ValueSetSet<const ValueSet *> valueSetsReachingValue(const Value &start, const ValueSetSet<VS> &values) {
	ValueSetsReachingValue<VS> explorer(values);
	explorer.backtrack(start);
	return std::move(explorer.result);
}


template class ValueSetsReachingValue<const ValueSet *>;
template class ValueSetsReachingValue<      ValueSet  >;

template ValueSetSet<const ValueSet *> valueSetsReachingValue<const ValueSet *>(const Value &, const ValueSetSet<const ValueSet *> &);
template ValueSetSet<const ValueSet *> valueSetsReachingValue<      ValueSet  >(const Value &, const ValueSetSet<      ValueSet  > &);
