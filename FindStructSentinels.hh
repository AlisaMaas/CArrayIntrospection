#ifndef INCLUDE_FIND_STRUCT_SENTINELS_HH
#define INCLUDE_FIND_STRUCT_SENTINELS_HH

#include <llvm/Pass.h>
#include <unordered_map>
#include "FindSentinelHelper.hh"

namespace llvm {
    class Argument;
}


typedef std::map<const StructElement, std::pair<BlockSet, bool>> ElementToBlockSet;

class FindStructSentinels : public llvm::ModulePass {
public:
    // standard LLVM pass interface
    FindStructSentinels();
    static char ID;
    void getAnalysisUsage(llvm::AnalysisUsage &) const final override;
    bool runOnModule(llvm::Module &) final override;
    void print(llvm::raw_ostream &, const llvm::Module *) const final override;

    // access to analysis results derived by this pass
    const FunctionResults *getResultsForFunction(const llvm::Function *) const;
    const std::unordered_map<const llvm::Function*, FunctionResults> getAllResults() const;
    const ValueSet* getValueSet(const StructElement &element) const;
    const StructElementToValueSet getElementToValueSet() const;
private:
    std::unordered_map<const llvm::Function *, FunctionResults> allSentinelChecks;
    StructElementToValueSet elementToValueSet;
};


////////////////////////////////////////////////////////////////////////

//TODO: Pull up into super class if this isn't the only removable shared code.
inline const FunctionResults *FindStructSentinels::getResultsForFunction(const llvm::Function *func) const {
    const auto found = allSentinelChecks.find(func);
    return found == allSentinelChecks.end() ? nullptr : &found->second;
}

inline const std::unordered_map<const llvm::Function *, FunctionResults>  FindStructSentinels::getAllResults() const {
    return allSentinelChecks;
}

inline const ValueSet* FindStructSentinels::getValueSet(const StructElement &element) const {
    return elementToValueSet.at(element);
}

inline const StructElementToValueSet FindStructSentinels::getElementToValueSet() const {
    return elementToValueSet;
}

#endif // !INCLUDE_FIND_STRUCT_SENTINELS_HH
