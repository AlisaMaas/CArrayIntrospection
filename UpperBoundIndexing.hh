#ifndef INCLUDE_UPPER_BOUND_INDEX_HH
#define INCLUDE_UPPER_BOUND_INDEX_HH

#include <llvm/Pass.h>

#include <unordered_map>
#include <unordered_set>

namespace llvm {
	class Argument;
}


typedef std::unordered_set<const llvm::BasicBlock *> BlockSet;
typedef std::unordered_map<const llvm::Argument *, std::pair<BlockSet, bool>> ArgumentToBlockSet;


class UpperBoundIndexing : public llvm::ModulePass {
public:
	// standard LLVM pass interface
	UpperBoundIndexing();
	static char ID;
	void getAnalysisUsage(llvm::AnalysisUsage &) const final;
	bool runOnModule(llvm::Module &) override final;
	void print(llvm::raw_ostream &, const llvm::Module *) const;
};




#endif // !INCLUDE_UPPER_BOUND_INDEX_HH
