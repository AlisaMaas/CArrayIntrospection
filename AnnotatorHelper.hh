#ifndef INCLUDE_ANNOTATOR_HELPER_HH
#define INCLUDE_ANNOTATOR_HELPER_HH

#include "AnnotationMap.hh"
#include "CallInstSet.hh"
#include "FindSentinelHelper.hh"
#include "LengthInfo.hh"

#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <memory>
#include <string>
#include <set>
#include <unordered_set>
#include <unordered_map>


typedef std::unordered_map<const llvm::Function*, std::unordered_set<std::shared_ptr<const ValueSet>>> FunctionToValueSets;
typedef std::unordered_map<llvm::Function*, std::vector<LoopInformation>> FunctionToLoopInformation;

LengthInfo findAssociatedAnswer(const llvm::Value *value, const AnnotationMap &annotations);
std::unordered_map<const llvm::Function *, CallInstSet> collectFunctionCalls(const llvm::Module &);
bool iterateOverModule(llvm::Module &module, const FunctionToValueSets &checkNullTerminated,
		       std::unordered_map<const llvm::Function *, CallInstSet> &functionToCallSites, AnnotationMap &annotations,
		       FunctionToLoopInformation &info, std::map<std::shared_ptr<const ValueSet>, std::string> &reasons, bool fast,
		       std::map<const llvm::Value *, std::shared_ptr<const ValueSet>> &valueToValueSet);
LengthInfo getAnswer(const std::shared_ptr<const ValueSet> &, const AnnotationMap &annotations);

#endif // !INCLUDE_NULL_ANNOTATOR_HELPER_HH
