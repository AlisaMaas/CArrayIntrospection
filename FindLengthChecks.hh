#ifndef INCLUDE_FIND_LENGTH_CHECKS_HH
#define INCLUDE_FIND_LENGTH_CHECKS_HH

#include <llvm/Pass.h>
#include <map>

namespace llvm {
	class Argument;
}

typedef std::map<llvm::Argument const*, int> ArgumentToMaxIndexMap;

class FindLengthChecks : public llvm::ModulePass {
public:
	// standard LLVM pass interface
	FindLengthChecks();
	static char ID;
	void getAnalysisUsage(llvm::AnalysisUsage &) const final;
	bool runOnModule(llvm::Module &) override final;
	void print(llvm::raw_ostream &, const llvm::Module *) const;
private:
	std::map<llvm::Function const*, ArgumentToMaxIndexMap> maxIndexes;
};




#endif // !INCLUDE_FIND_LENGTH_CHECKS_HH
