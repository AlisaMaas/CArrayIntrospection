#ifndef INCLUDE_FIND_LENGTH_CHECKS_HH
#define INCLUDE_FIND_LENGTH_CHECKS_HH

#include "LengthValueSetMap.hh"
#include "ValueSetSet.hh"
#include "ValueSetToMaxIndexMap.hh"

#include <llvm/Pass.h>
#include <map>
#include <memory>


class FindLengthChecks : public llvm::ModulePass {
public:
	// standard LLVM pass interface
	FindLengthChecks();
	static char ID;
	void getAnalysisUsage(llvm::AnalysisUsage &) const final;
	bool runOnModule(llvm::Module &) override final;
	void print(llvm::raw_ostream &, const llvm::Module *) const override;

private:
	std::map<llvm::Function const *, ValueSetToMaxIndexMap> maxIndexes;
	std::map<llvm::Function const *, LengthValueSetMap> lengths;
	ValueSetSet valueSets;
};


#endif // !INCLUDE_FIND_LENGTH_CHECKS_HH
