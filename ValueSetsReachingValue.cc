#include "BacktrackPhiNodes.hh"
#include "ValueSetsReachingValue.hh"

#include <llvm/Support/Casting.h>
#include <memory>

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
		void visit(const llvm::Value &) final override;
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


ValueSetSet valueSetsReachingValue(const llvm::Value &start, const ValueSetSet &values)
{
	ValueSetsReachingValue explorer{values};
	explorer.backtrack(start);
	return move(explorer.result);
}
