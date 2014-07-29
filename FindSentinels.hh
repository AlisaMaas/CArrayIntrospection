#ifndef INCLUDE_FIND_SENTINELS_HH
#define INCLUDE_FIND_SENTINELS_HH
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <unordered_map>
#include <unordered_set>
typedef std::unordered_set<const llvm::BasicBlock *> BlockSet;

namespace {
	class FindSentinels : public llvm::FunctionPass {
	public:
		FindSentinels();
		static char ID;
		void getAnalysisUsage(llvm::AnalysisUsage &) const final;
		bool runOnFunction(llvm::Function &) override final;
		void print(llvm::raw_ostream &, const llvm::Module *) const;
	private:
		std::unordered_map<llvm::Function *, std::unordered_map<llvm::BasicBlock const *, std::pair<BlockSet, bool>>> allSentinelChecks;
		llvm::Function * current; 
	};

	char FindSentinels::ID;
}

#endif // !INCLUDE_FIND_SENTINELS_HH
