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

template <typename VS>
class ValueSetsReachingValue : public BacktrackPhiNodes {
public:
	ValueSetsReachingValue(const ValueSetSet<VS> &values);
	void visit(const llvm::Value &) final override;
	ValueSetSet<const ValueSet *> result;
private:
	const ValueSetSet<VS> &valueSets;
};


template <typename VS>
ValueSetSet<const ValueSet *> valueSetsReachingValue(const llvm::Value &, const ValueSetSet<VS> &);


////////////////////////////////////////////////////////////////////////


template <typename VS>
inline ValueSetsReachingValue<VS>::ValueSetsReachingValue(const ValueSetSet<VS> &values)
	: valueSets(values)
{
}


#endif	// !INCLUDE_ARGUMENTS_REACHING_VALUE_HH
