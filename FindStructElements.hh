#ifndef INCLUDE_FIND_STRUCT_ELEMENTS_HH
#define INCLUDE_FIND_STRUCT_ELEMENTS_HH

#include "FindSentinelHelper.hh"
#include "StructElement.hh"

#include <llvm/Pass.h>
#include <map>
#include <memory>
#include <vector>

typedef std::map<StructElement, std::shared_ptr<ValueSet>> StructElementToValueSet;


class FindStructElements : public llvm::ModulePass {
public:
	// standard LLVM pass interface
	FindStructElements();
	static char ID;
	void getAnalysisUsage(llvm::AnalysisUsage &) const final;
	bool runOnModule(llvm::Module &) override final;
	void print(llvm::raw_ostream &, const llvm::Module *) const override;
	// access to analysis results derived by this pass
	const StructElementToValueSet &getStructElements() const;
	const std::vector<const llvm::StructType *> &getAllStructs() const;
private:
	StructElementToValueSet structElementCollections;
	std::vector<const llvm::StructType *> orderedTypes;
};

////////////////////////////////////////////////////////////////////////


inline const StructElementToValueSet &FindStructElements::getStructElements() const {
	return structElementCollections;
}

inline const std::vector<const llvm::StructType*> &FindStructElements::getAllStructs() const {
	return orderedTypes;
}


#endif // !INCLUDE_FIND_STRUCT_ELEMENTS_HH
