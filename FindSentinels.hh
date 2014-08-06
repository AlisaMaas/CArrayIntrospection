#ifndef INCLUDE_FIND_SENTINELS_HH
#define INCLUDE_FIND_SENTINELS_HH

#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <unordered_map>
#include <unordered_set>

namespace llvm {
	class Argument;
}


typedef std::unordered_set<const llvm::BasicBlock *> BlockSet;
typedef std::unordered_map<const llvm::Argument *, std::pair<BlockSet, bool>> ArgumentToBlockSet;

class FindSentinels : public llvm::ModulePass {
public:
	FindSentinels();
	static char ID;
	void getAnalysisUsage(llvm::AnalysisUsage &) const final;
	const std::unordered_map<llvm::BasicBlock const *, ArgumentToBlockSet>& getResultsForFunction(const llvm::Function*) const;
	bool runOnModule(llvm::Module &) override final;
	void print(llvm::raw_ostream &, const llvm::Module *) const;
private:
	std::unordered_map<const llvm::Function *, std::unordered_map<const llvm::BasicBlock *, ArgumentToBlockSet>> allSentinelChecks;
};

#endif // !INCLUDE_FIND_SENTINELS_HH
