#ifndef INCLUDE_FIND_SENTINELS_HH
#define INCLUDE_FIND_SENTINELS_HH

#include <llvm/Pass.h>
#include <unordered_map>
#include "FindSentinelHelper.hh"

namespace llvm {
	class Argument;
}


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
	const FunctionResults *getResultsForFunction(const llvm::Function *) const;
	const std::unordered_map<const llvm::Function*, FunctionResults> getAllResults() const;

private:
	std::unordered_map<const llvm::Function *, FunctionResults> allSentinelChecks;
};


////////////////////////////////////////////////////////////////////////

//TODO: Pull up into super class if this isn't the only removable shared code.
inline const FunctionResults *FindSentinels::getResultsForFunction(const llvm::Function *func) const {
	const auto found = allSentinelChecks.find(func);
	return found == allSentinelChecks.end() ? nullptr : &found->second;
}

inline const std::unordered_map<const llvm::Function *, FunctionResults>  FindSentinels::getAllResults() const {
	return allSentinelChecks;
}

#endif // !INCLUDE_FIND_SENTINELS_HH
