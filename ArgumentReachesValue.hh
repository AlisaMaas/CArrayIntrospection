#include "BacktrackPhiNodes.hh"

#include <llvm/IR/Module.h>

////////////////////////////////////////////////////////////////////////
//
//  test whether a specific argument may flow into a specific value
//  across zero or more phi nodes
//

class ArgumentReachesValue : public BacktrackPhiNodes {
public:
	ArgumentReachesValue(const llvm::Value &);
	void visit(const llvm::Value &) final override;
private:
	const llvm::Value &goal;
};

inline ArgumentReachesValue::ArgumentReachesValue(const llvm::Value &goal)
	: goal(goal) {
}


void ArgumentReachesValue::visit(const llvm::Value &reached) {
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
