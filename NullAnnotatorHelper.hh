#ifndef INCLUDE_NULL_ANNOTATOR_HELPER_HH
#define INCLUDE_NULL_ANNOTATOR_HELPER_HH

#include "Answer.hh"
#include "FindSentinelHelper.hh"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <string>
#include <set>
#include <unordered_set>
#include <unordered_map>

typedef std::unordered_set<const llvm::CallInst *> CallInstSet;
typedef std::unordered_map<const ValueSet*, Answer> AnnotationMap;
typedef std::unordered_map<const llvm::Function*, std::unordered_set<const ValueSet*>> FunctionToValueSets;
typedef std::unordered_map<llvm::Function*, std::vector<LoopInformation>> FunctionToLoopInformation;

Answer findAssociatedAnswer(const llvm::Value *value, const AnnotationMap &annotations);
bool annotate(const llvm::Value &, AnnotationMap &annotations);
std::unordered_map<const llvm::Function *, CallInstSet> collectFunctionCalls(const llvm::Module &);
bool iterateOverModule(llvm::Module &module, const FunctionToValueSets &checkNullTerminated, 
	std::unordered_map<const llvm::Function *, CallInstSet> &functionToCallSites, AnnotationMap &annotations,
	FunctionToLoopInformation &info, std::map<const ValueSet, std::string> &reasons, bool fast=false);
Answer getAnswer(const ValueSet &, const AnnotationMap &annotations);

#endif // !INCLUDE_NULL_ANNOTATOR_HELPER_HH
