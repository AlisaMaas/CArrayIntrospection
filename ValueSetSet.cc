#include "BacktrackPhiNodes.hh"
#include "ValueSetSet.hh"

using namespace llvm;
using namespace std;


namespace {
	////////////////////////////////////////////////////////////////
	//
	//  collect the set of all arguments that may flow to a given value
	//  across zero or more phi nodes
	//

	class ValueSetsReachingValue : public BacktrackPhiNodes {
	public:
		ValueSetsReachingValue(const ValueSetSet &values);
		void visit(const Value &) final override;
		ValueSetSet result;
	private:
		const ValueSetSet &valueSets;
	};
}


inline ValueSetsReachingValue::ValueSetsReachingValue(const ValueSetSet &values)
	: valueSets{values}
{
}


void ValueSetsReachingValue::visit(const Value &reached) {
	const shared_ptr<const ValueSet> valueSet{valueSets.getValueSetFromValue(&reached)};
	if (valueSet)
		result.insert(valueSet);
}


std::shared_ptr<const ValueSet> ValueSetSet::getValueSetFromValue(const Value *value) const {
        for (const auto &valueSet : *this)
		if (valueSet && valueSet->count(value))
			return valueSet;

        return { };
}


ValueSetSet ValueSetSet::valueSetsReachingValue(const Value &start) const
{
	ValueSetsReachingValue explorer{*this};
	explorer.backtrack(start);
	return move(explorer.result);
}
