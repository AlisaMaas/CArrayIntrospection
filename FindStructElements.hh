#ifndef INCLUDE_FIND_STRUCT_ELEMENTS_HH
#define INCLUDE_FIND_STRUCT_ELEMENTS_HH

#include <llvm/Pass.h>
#include <map>
#include <vector>
#include "FindSentinelHelper.hh"

typedef std::map<std::pair<llvm::StructType*, int>, ValueSet> StructElementToValueSet;

class FindStructElements : public llvm::ModulePass {
public:
	// standard LLVM pass interface
	FindStructElements();
	static char ID;
	void getAnalysisUsage(llvm::AnalysisUsage &) const final;
	bool runOnModule(llvm::Module &) override final;
	void print(llvm::raw_ostream &, const llvm::Module *) const;
	// access to analysis results derived by this pass
	const StructElementToValueSet getStructElements() const;
private:
	StructElementToValueSet structElementCollections;
	std::vector<llvm::StructType*> orderedTypes;
};

////////////////////////////////////////////////////////////////////////

inline const StructElementToValueSet FindStructElements::getStructElements() const {
	return structElementCollections;
}

#endif // !INCLUDE_FIND_STRUCT_ELEMENTS_HH
