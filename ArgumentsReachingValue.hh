#ifndef INCLUDE_ARGUMENTS_REACHING_VALUE_HH
#define INCLUDE_ARGUMENTS_REACHING_VALUE_HH

#include "BacktrackPhiNodes.hh"

#include <llvm/Support/Casting.h>
#include <unordered_set>


////////////////////////////////////////////////////////////////////////
//
//  collect the set of all arguments that may flow to a given value
//  across zero or more phi nodes
//

namespace llvm {
	class Argument;
}

typedef std::unordered_set<const llvm::Argument *> ArgumentSet;

class ArgumentsReachingValue : public BacktrackPhiNodes {
public:
	void visit(const llvm::Value &) final override;
	ArgumentSet result;
	static ArgumentSet argumentsReachingValue(const llvm::Value &start);
};

inline void ArgumentsReachingValue::visit(const llvm::Value &reached) {
	result.insert(llvm::dyn_cast<llvm::Argument>(&reached));
}

inline static ArgumentSet argumentsReachingValue(const llvm::Value &start) {
	ArgumentsReachingValue explorer;
	explorer.backtrack(start);
	return std::move(explorer.result);
}

#endif	// !INCLUDE_ARGUMENTS_REACHING_VALUE_HH
