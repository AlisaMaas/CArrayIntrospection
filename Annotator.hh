#ifndef INCLUDE_ANNOTATOR_HH
#define INCLUDE_ANNOTATOR_HH

#include "AnnotationMap.hh"
#include "CallInstSet.hh"
#include "FindLengthChecks.hh"
#include "FindStructElements.hh"
#include "IIGlueReader.hh"
#include "LengthInfo.hh"
#include "ValueSetsReachingValue.hh"

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <map>
#include <memory>
#include <unordered_map>
#include <unordered_set>


class Annotator : public llvm::ModulePass {
public:
	// standard LLVM pass interface
	Annotator();
	static char ID;
	void getAnalysisUsage(llvm::AnalysisUsage &) const final override;
	bool runOnModule(llvm::Module &) final override;
	void print(llvm::raw_ostream &, const llvm::Module *) const final override;
	bool doFinalization(llvm::Module &) final override;

	// access to analysis results derived by this pass
	std::pair<int,int> annotate(const llvm::Value &) const;
	llvm::LoopInfo& runLoopInfo(llvm::Function &func);

private:
	// map from function name and argument number to whether or not that argument gets annotated
	AnnotationMap annotations;
	std::unordered_map<const llvm::Argument *, std::shared_ptr<ValueSet>> argumentToValueSet;
	std::map<std::shared_ptr<const ValueSet>, std::string> reasons;
	std::unordered_map<const llvm::Function *, CallInstSet> functionToCallSites;
	const StructElementToValueSet *structElements;
	std::pair<int,int> annotate(const StructElement &element) const;
	void dumpToFile(const std::string &filename, const llvm::Module &) const;
	void populateFromFile(const std::string &filename, const llvm::Module &);
	std::pair<int, int> annotate(const LengthInfo &info) const;
	std::map<llvm::Function const*, ValueSetToMaxIndexMap> maxIndexes;
	std::map<llvm::Function const*, LengthValueSetMap> lengths;
};

#endif // !INCLUDE_NULL_ANNOTATOR_HH
