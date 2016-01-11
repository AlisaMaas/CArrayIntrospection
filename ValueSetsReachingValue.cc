#include "BacktrackPhiNodes.hh"
#include "ValueSetsReachingValue.hh"

#include <llvm/Support/Casting.h>

using namespace llvm;
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


////////////////////////////////////////////////////////////////////////


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
	return std::move(explorer.result);
}


template class ValueSetsReachingValue<const ValueSet *>;
template class ValueSetsReachingValue<      ValueSet  >;

template ValueSetSet<const ValueSet *> valueSetsReachingValue<const ValueSet *>(const Value &, const ValueSetSet<const ValueSet *> &);
template ValueSetSet<const ValueSet *> valueSetsReachingValue<      ValueSet  >(const Value &, const ValueSetSet<      ValueSet  > &);
