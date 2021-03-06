#ifndef INCLUDE_CHECK_GET_ELEMENT_PTR_VISITOR_HH
#define INCLUDE_CHECK_GET_ELEMENT_PTR_VISITOR_HH

#include "AnnotatorHelper.hh"
#include "CallInstSet.hh"
#include "LengthValueSetMap.hh"
#include "ValueSetSet.hh"
#include "ValueSetToMaxIndexMap.hh"

#include <llvm/IR/InstVisitor.h>
#include <memory>
#include <unordered_map>

class SymbolicRangeAnalysis;


struct CheckGetElementPtrVisitor : public llvm::InstVisitor<CheckGetElementPtrVisitor> {
public:
	ValueSetToMaxIndexMap &maxIndexes;
	LengthValueSetMap &lengths;
	CheckGetElementPtrVisitor(ValueSetToMaxIndexMap &, const SymbolicRangeAnalysis &ra,
				  const llvm::Module &m, LengthValueSetMap &, const ValueSetSet &v);
	~CheckGetElementPtrVisitor();
	void visitGetElementPtrInst(llvm::GetElementPtrInst& gepi);
	ValueSetSet notConstantBounded;
	ValueSetSet notParameterBounded;
private:
	const std::shared_ptr<const ValueSet> getValueLength(llvm::Value *first, llvm::Value *second, const llvm::Value *basePointer);
	bool matchAddPattern(llvm::Value *value, llvm::Value *basePointer);
	const SymbolicRangeAnalysis &rangeAnalysis;
	const ValueSetSet &valueSets;
	const llvm::Module &module;
	std::unique_ptr<llvm::BasicBlock> placeHolder;
	std::unordered_map<const llvm::Function *, CallInstSet> functionsToCallsites;
};


#endif	// !INCLUDE_CHECK_GET_ELEMENT_PTR_VISITOR_HH
