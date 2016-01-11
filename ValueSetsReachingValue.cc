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

	template <typename VS>
	class ValueSetsReachingValue : public BacktrackPhiNodes {
	public:
		ValueSetsReachingValue(const ValueSetSet<VS> &values);
		void visit(const llvm::Value &) final override;
		ValueSetSet<const ValueSet *> result;
	private:
		const ValueSetSet<VS> &valueSets;
	};
}


template <typename VS>
inline ValueSetsReachingValue<VS>::ValueSetsReachingValue(const ValueSetSet<VS> &values)
	: valueSets(values)
{
}


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
	return move(explorer.result);
}


template ValueSetSet<const ValueSet *> valueSetsReachingValue<const ValueSet *>(const Value &, const ValueSetSet<const ValueSet *> &);
template ValueSetSet<const ValueSet *> valueSetsReachingValue<      ValueSet  >(const Value &, const ValueSetSet<      ValueSet  > &);


////////////////////////////////////////////////////////////////////////


namespace {
	////////////////////////////////////////////////////////////////
	//
	//  collect the set of all arguments that may flow to a given value
	//  across zero or more phi nodes
	//

	class SharedValueSetsReachingValue : public BacktrackPhiNodes {
	public:
		SharedValueSetsReachingValue(const ValueSetSet<shared_ptr<const ValueSet>> &values);
		void visit(const llvm::Value &) final override;
		ValueSetSet<shared_ptr<const ValueSet>> result;
	private:
		const ValueSetSet<shared_ptr<const ValueSet>> &valueSets;
	};
}


inline SharedValueSetsReachingValue::SharedValueSetsReachingValue(const ValueSetSet<shared_ptr<const ValueSet>> &values)
	: valueSets{values}
{
}


void SharedValueSetsReachingValue::visit(const Value &reached) {
	const shared_ptr<const ValueSet> valueSet{valueSets.getValueSetFromValue(&reached)};
	if (valueSet)
		result.insert(valueSet);
}


ValueSetSet<shared_ptr<const ValueSet>> valueSetsReachingValue(const llvm::Value &start, const ValueSetSet<shared_ptr<const ValueSet>> &values)
{
	SharedValueSetsReachingValue explorer{values};
	explorer.backtrack(start);
	return move(explorer.result);
}
