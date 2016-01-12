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


template <typename VS>
struct CheckGetElementPtrVisitor : public llvm::InstVisitor<CheckGetElementPtrVisitor<VS>> {
public:
	ValueSetToMaxIndexMap &maxIndexes;
	LengthValueSetMap &lengths;
	CheckGetElementPtrVisitor(ValueSetToMaxIndexMap &map, const SymbolicRangeAnalysis &ra,
				  const llvm::Module &m, LengthValueSetMap &l, const ValueSetSet<VS> &v);
	~CheckGetElementPtrVisitor();
	void visitGetElementPtrInst(llvm::GetElementPtrInst& gepi);
	ValueSetSet<const ValueSet *> notConstantBounded;
	ValueSetSet<const ValueSet *> notParameterBounded;
private:
	const ValueSet *getValueLength(llvm::Value *first, llvm::Value *second, const llvm::Value *basePointer);
	bool matchAddPattern(llvm::Value *value, llvm::Value *basePointer);
	const SymbolicRangeAnalysis &rangeAnalysis;
	const ValueSetSet<VS> &valueSets;
	const llvm::Module &module;
	std::unique_ptr<llvm::BasicBlock> placeHolder;
	std::unordered_map<const llvm::Function *, CallInstSet> functionsToCallsites;
};


struct SharedCheckGetElementPtrVisitor : public llvm::InstVisitor<SharedCheckGetElementPtrVisitor> {
public:
	SharedValueSetToMaxIndexMap &maxIndexes;
	SharedLengthValueSetMap &lengths;
	SharedCheckGetElementPtrVisitor(SharedValueSetToMaxIndexMap &, const SymbolicRangeAnalysis &ra,
				  const llvm::Module &m, SharedLengthValueSetMap &, const ValueSetSet<std::shared_ptr<const ValueSet>> &v);
	~SharedCheckGetElementPtrVisitor();
	void visitGetElementPtrInst(llvm::GetElementPtrInst& gepi);
	ValueSetSet<std::shared_ptr<const ValueSet>> notConstantBounded;
	ValueSetSet<std::shared_ptr<const ValueSet>> notParameterBounded;
private:
	const std::shared_ptr<const ValueSet> getValueLength(llvm::Value *first, llvm::Value *second, const llvm::Value *basePointer);
	bool matchAddPattern(llvm::Value *value, llvm::Value *basePointer);
	const SymbolicRangeAnalysis &rangeAnalysis;
	const ValueSetSet<std::shared_ptr<const ValueSet>> &valueSets;
	const llvm::Module &module;
	std::unique_ptr<llvm::BasicBlock> placeHolder;
	std::unordered_map<const llvm::Function *, CallInstSet> functionsToCallsites;
};


#endif	// !INCLUDE_CHECK_GET_ELEMENT_PTR_VISITOR_HH
