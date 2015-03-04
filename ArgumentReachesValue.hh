#include <llvm/IR/Module.h>

#include "BacktrackPhiNodes.hh"

////////////////////////////////////////////////////////////////////////
//
//  test whether a specific argument may flow into a specific value
//  across zero or more phi nodes
//

namespace {
	class ArgumentReachesValue : public BacktrackPhiNodes {
	public:
		ArgumentReachesValue(const llvm::Argument &);
		void visit(const llvm::Argument &) final override;

	private:
		const llvm::Argument &goal;
	};
}


inline ArgumentReachesValue::ArgumentReachesValue(const llvm::Argument &goal)
	: goal(goal) {
}


void ArgumentReachesValue::visit(const llvm::Argument &reached) {
	if (&reached == &goal)
		throw this;
}


static bool argumentReachesValue(const llvm::Argument &goal, const llvm::Value &start) {
	ArgumentReachesValue explorer(goal);
	try {
		explorer.backtrack(start);
	} catch (const ArgumentReachesValue *) {
		return true;
	}
	return false;
}


////////////////////////////////////////////////////////////////////////