#ifndef INCLUDE_FIND_LENGTH_CHECKS_HH
#define INCLUDE_FIND_LENGTH_CHECKS_HH

#include "SRA/SymbolicRangeAnalysis.h"
#include "LengthValueSetMap.hh"
#include "ValueSetSet.hh"
#include "ValueSetToMaxIndexMap.hh"

#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>
#include <map>
#include <unordered_map>
#include <unordered_set>

typedef std::pair<const ValueSetToMaxIndexMap *, const LengthValueSetMap *> FunctionLengthResults;


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
	std::map<llvm::Function const*, ValueSetToMaxIndexMap> maxIndexes;
	std::map<llvm::Function const*, LengthValueSetMap> lengths;
	ValueSetSet valueSets;
};

struct CheckGetElementPtrVisitor : public llvm::InstVisitor<CheckGetElementPtrVisitor> {
    public:
        ValueSetToMaxIndexMap &maxIndexes;
        LengthValueSetMap &lengths;
	    CheckGetElementPtrVisitor(ValueSetToMaxIndexMap &map, 
	        const SymbolicRangeAnalysis &ra, llvm::Module &m, LengthValueSetMap &l, ValueSetSet &v );
	    ~CheckGetElementPtrVisitor();
    	void visitGetElementPtrInst(llvm::GetElementPtrInst& gepi);
    	ValueSetSet notConstantBounded;
        ValueSetSet notParameterBounded;
    private: 
        const ValueSet *getValueLength(llvm::Value *first, llvm::Value *second, const llvm::Value *basePointer);
        bool matchAddPattern(llvm::Value *value, llvm::Value *basePointer);
        const SymbolicRangeAnalysis &rangeAnalysis;
        ValueSetSet valueSets;
        llvm::Module &module;
        llvm::BasicBlock *placeHolder;
        std::unordered_map<const llvm::Function *, CallInstSet> functionsToCallsites;
};


#endif // !INCLUDE_FIND_LENGTH_CHECKS_HH
