#ifndef INCLUDE_FIND_STRUCT_ELEMENTS_HH
#define INCLUDE_FIND_STRUCT_ELEMENTS_HH

#include <llvm/Pass.h>
#include <map>
#include <vector>
#include "FindSentinelHelper.hh"

typedef std::pair<llvm::StructType*, int> StructElement;
typedef std::map<StructElement, ValueSet*> StructElementToValueSet;

class FindStructElements : public llvm::ModulePass {
public:
    // standard LLVM pass interface
    FindStructElements();
    static char ID;
    void getAnalysisUsage(llvm::AnalysisUsage &) const final;
    bool runOnModule(llvm::Module &) override final;
    void print(llvm::raw_ostream &, const llvm::Module *) const override;
    // access to analysis results derived by this pass
    const StructElementToValueSet getStructElements() const;
    const std::vector<llvm::StructType*> getAllStructs() const;
private:
    StructElementToValueSet structElementCollections;
    std::vector<llvm::StructType*> orderedTypes;
};

////////////////////////////////////////////////////////////////////////

StructElement* getStructElement(const llvm::Value *value);

std::string str(const StructElement *element);

inline const StructElementToValueSet FindStructElements::getStructElements() const {
    return structElementCollections;
}

inline const std::vector<llvm::StructType*> FindStructElements::getAllStructs() const {
    return orderedTypes;
}


#endif // !INCLUDE_FIND_STRUCT_ELEMENTS_HH
