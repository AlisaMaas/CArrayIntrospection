#include "ArgumentReachesValue.hh"


namespace {
	class ArgumentReachesValue : public BacktrackPhiNodes {
	public:
		ArgumentReachesValue(const llvm::Value &);
		void visit(const llvm::Value &) final override;
	private:
		const llvm::Value &goal;
	};
}


inline ArgumentReachesValue::ArgumentReachesValue(const llvm::Value &goal)
	: goal(goal) {
}


inline void ArgumentReachesValue::visit(const llvm::Value &reached) {
	if (&reached == &goal)
		throw this;
}


bool argumentReachesValue(const llvm::Argument &goal, const llvm::Value &start) {
	ArgumentReachesValue explorer(goal);
	try {
		explorer.backtrack(start);
	} catch (const ArgumentReachesValue *) {
		return true;
	}
	return false;
}
