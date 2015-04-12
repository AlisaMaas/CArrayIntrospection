#ifndef INCLUDE_NULL_ANNOTATOR_HELPER_HH
#define INCLUDE_NULL_ANNOTATOR_HELPER_HH

#include "Answer.hh"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <string>
#include <unordered_set>
#include <unordered_map>

typedef std::unordered_set<const llvm::CallInst *> CallInstSet;
typedef std::unordered_map<const llvm::Value *, Answer> AnnotationMap;
typedef std::unordered_map<const llvm::Function*, std::unordered_set<const llvm::Value*>> FunctionToValues;

bool annotate(const llvm::Value &, AnnotationMap &annotations);
std::unordered_map<const llvm::Function *, CallInstSet> collectFunctionCalls(const llvm::Module &);
bool processLoops(const llvm::Module &module, const FunctionToValues &checkNullTerminated, 
	std::unordered_map<const llvm::Function *, FunctionResults> const &allSentinelChecks,
	AnnotationMap &annotations, std::unordered_map<const llvm::Value *, std::string> &reasons);
bool iterateOverModule(const llvm::Module &module, const FunctionToValues &checkNullTerminated, 
	std::unordered_map<const llvm::Function *, FunctionResults> const &allSentinelChecks,
	std::unordered_map<const llvm::Function *, CallInstSet> &functionToCallSites, AnnotationMap &annotations,
	std::unordered_map<const llvm::Value *, std::string> &reasons);
Answer getAnswer(const llvm::Value &value, const AnnotationMap &annotations);

#endif // !INCLUDE_NULL_ANNOTATOR_HELPER_HH
