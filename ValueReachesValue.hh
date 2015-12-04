#include "BacktrackPhiNodes.hh"

#include <llvm/IR/Module.h>

////////////////////////////////////////////////////////////////////////
//
//  test whether a specific argument may flow into a specific value
//  across zero or more phi nodes
//

	class ValueReachesValue : public BacktrackPhiNodes {
	public:
		ValueReachesValue(const llvm::Value &);
		void visit(const llvm::Value &) final override;
		bool shouldVisit(const llvm::Value &) final override;
	private:
		const llvm::Value &goal;
	};


inline ValueReachesValue::ValueReachesValue(const llvm::Value &goal)
	: goal(goal) {
}

inline void ValueReachesValue::visit(const llvm::Value &reached) {
	if (&reached == &goal) {
		throw this;
	}
}

inline bool ValueReachesValue::shouldVisit(const llvm::Value &) {
	return true;
}

inline bool valueReachesValue(const llvm::Value &goal, const llvm::Value &start, bool skipLoads=false) {
	ValueReachesValue explorer(goal);
	try {
		explorer.backtrack(start, skipLoads);
	} catch (const ValueReachesValue *) {
		return true;
	}
	return false;
}


////////////////////////////////////////////////////////////////////////
