#ifndef INCLUDE_FIND_LENGTH_CHECKS_HH
#define INCLUDE_FIND_LENGTH_CHECKS_HH

#include <llvm/Pass.h>
#include <map>

namespace llvm {
	class Argument;
}

typedef std::map<llvm::Argument const*, long int> ArgumentToMaxIndexMap;
typedef std::map<llvm::Argument const*, llvm::Argument const*> LengthArgumentMap;
typedef std::pair<const ArgumentToMaxIndexMap*, const LengthArgumentMap*> FunctionLengthResults;

class FindLengthChecks : public llvm::ModulePass {
public:
	// standard LLVM pass interface
	FindLengthChecks();
	static char ID;
	void getAnalysisUsage(llvm::AnalysisUsage &) const final;
	bool runOnModule(llvm::Module &) override final;
	void print(llvm::raw_ostream &, const llvm::Module *) const override;
	// access to analysis results derived by this pass
	const FunctionLengthResults getResultsForFunction(const llvm::Function *) const;
private:
	std::map<llvm::Function const*, ArgumentToMaxIndexMap> maxIndexes;
	std::map<llvm::Function const*, LengthArgumentMap> lengthArguments;
};

////////////////////////////////////////////////////////////////////////

inline const FunctionLengthResults FindLengthChecks::getResultsForFunction(const llvm::Function *func) const {
	const ArgumentToMaxIndexMap* first;
	const LengthArgumentMap* second;
	const auto findConst = maxIndexes.find(func);
	first = (findConst == maxIndexes.end() ? nullptr : &findConst->second);

	const auto findParam = lengthArguments.find(func);
	second = (findParam == lengthArguments.end() ? nullptr : &findParam->second);
	return std::pair<const ArgumentToMaxIndexMap*, const LengthArgumentMap*>(first, second);
}

#endif // !INCLUDE_FIND_LENGTH_CHECKS_HH
