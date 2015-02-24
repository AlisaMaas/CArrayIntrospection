#ifndef INCLUDE_FIND_LENGTH_CHECKS_HH
#define INCLUDE_FIND_LENGTH_CHECKS_HH

#include <llvm/Pass.h>
#include <map>

namespace llvm {
	class Argument;
}

typedef std::map<llvm::Argument const*, long int> ArgumentToMaxIndexMap;
typedef std::map<llvm::Argument const*, llvm::Argument const*> LengthArgumentMap;

class FindLengthChecks : public llvm::ModulePass {
public:
	// standard LLVM pass interface
	FindLengthChecks();
	static char ID;
	void getAnalysisUsage(llvm::AnalysisUsage &) const final;
	bool runOnModule(llvm::Module &) override final;
	void print(llvm::raw_ostream &, const llvm::Module *) const;
	// access to analysis results derived by this pass
	typedef std::pair<ArgumentToMaxIndexMap*, LengthArgumentMap*> FunctionLengthResults;
	const FunctionLengthResults getResultsForFunction(const llvm::Function *) const;
private:
	std::map<llvm::Function const*, ArgumentToMaxIndexMap> maxIndexes;
	std::map<llvm::Function const*, LengthArgumentMap> lengthArguments;
};

////////////////////////////////////////////////////////////////////////

inline const FindLengthChecks::FunctionLengthResults FindLengthChecks::getResultsForFunction(const llvm::Function *func) const {
	ArgumentToMaxIndexMap* first;
	LengthArgumentMap* second;
	const auto findConst = maxIndexes.find(func);
	findConst == maxIndexes.end() ? nullptr : &findConst->second;

	const auto findParam = lengthArguments.find(func);
	findParam == lengthArguments.end() ? nullptr : &findConst->second;
	return std::pair<ArgumentToMaxIndexMap*, LengthArgumentMap*>(first, second);
}

#endif // !INCLUDE_FIND_LENGTH_CHECKS_HH
