#ifndef INCLUDE_FIND_SENTINELS_HH
#define INCLUDE_FIND_SENTINELS_HH

#include <llvm/Pass.h>

#include <unordered_map>
#include <unordered_set>

namespace llvm {
	class Argument;
}


typedef std::unordered_set<const llvm::BasicBlock *> BlockSet;
typedef std::unordered_map<const llvm::Argument *, std::pair<BlockSet, bool>> ArgumentToBlockSet;


class FindSentinels : public llvm::ModulePass {
public:
	// standard LLVM pass interface
	FindSentinels();
	static char ID;
	void getAnalysisUsage(llvm::AnalysisUsage &) const final override;
	bool runOnModule(llvm::Module &) final override;
	void print(llvm::raw_ostream &, const llvm::Module *) const final override;

	// access to analysis results derived by this pass
	typedef std::unordered_map<const llvm::BasicBlock *, ArgumentToBlockSet> FunctionResults;
	const FunctionResults *getResultsForFunction(const llvm::Function *) const;

private:
	std::unordered_map<const llvm::Function *, FunctionResults> allSentinelChecks;
};


////////////////////////////////////////////////////////////////////////


inline const FindSentinels::FunctionResults *FindSentinels::getResultsForFunction(const llvm::Function *func) const {
	const auto found = allSentinelChecks.find(func);
	return found == allSentinelChecks.end() ? nullptr : &found->second;
}


#endif // !INCLUDE_FIND_SENTINELS_HH
