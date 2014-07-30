#ifndef INCLUDE_FIND_SENTINELS_HH
#define INCLUDE_FIND_SENTINELS_HH

#include <llvm/IR/Argument.h>
#include <llvm/Pass.h>
#include <llvm/Support/raw_ostream.h>

#include <unordered_map>
#include <unordered_set>
typedef std::unordered_set<const llvm::BasicBlock *> BlockSet;
typedef std::unordered_map<llvm::Argument *, std::pair<BlockSet, bool>> ArgumentToBlockSet;

namespace {
	class FindSentinels : public llvm::FunctionPass {
	public:
		FindSentinels();
		static char ID;
		void getAnalysisUsage(llvm::AnalysisUsage &) const final;
		std::unordered_map<llvm::BasicBlock const *, ArgumentToBlockSet> getResultsForFunction(llvm::Function*) const;
		bool runOnFunction(llvm::Function &) override final;
		void print(llvm::raw_ostream &, const llvm::Module *) const;
	private:
		std::unordered_map<llvm::Function *, std::unordered_map<llvm::BasicBlock const *, ArgumentToBlockSet>> allSentinelChecks;
		llvm::Function * current; 
	};

	char FindSentinels::ID;
}

inline std::unordered_map<llvm::BasicBlock const *, ArgumentToBlockSet> FindSentinels::getResultsForFunction(llvm::Function *func) const{
	return allSentinelChecks.at(func);
};

#endif // !INCLUDE_FIND_SENTINELS_HH
