#ifndef INCLUDE_ANNOTATOR_HH
#define INCLUDE_ANNOTATOR_HH

#include "ValueSetsReachingValue.hh"
#include "IIGlueReader.hh"
#include "FindStructElements.hh"
#include "LengthInfo.hh"

#include <llvm/IR/Module.h>
#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <map>
#include <unordered_map>
#include <unordered_set>

namespace {
    typedef std::unordered_map<const ValueSet*, LengthInfo> AnnotationMap;
    typedef std::unordered_set<const llvm::CallInst *> CallInstSet;


	class Annotator : public llvm::ModulePass {
	public:
		// standard LLVM pass interface
		Annotator();
		static char ID;
		void getAnalysisUsage(llvm::AnalysisUsage &) const final override;
		bool runOnModule(llvm::Module &) final override;
		void print(llvm::raw_ostream &, const llvm::Module *) const final override;

		// access to analysis results derived by this pass
		std::pair<int,int> annotate(const llvm::Value &) const;
		llvm::LoopInfo& runLoopInfo(llvm::Function &func);

	private:
		// map from function name and argument number to whether or not that argument gets annotated
		AnnotationMap annotations;
		std::unordered_map<const llvm::Argument *, const ValueSet*> argumentToValueSet;
		std::map<const ValueSet, std::string> reasons;
		std::unordered_map<const llvm::Function *, CallInstSet> functionToCallSites;
		StructElementToValueSet structElements;
		std::pair<int,int> annotate(const StructElement &element) const;
		void dumpToFile(const std::string &filename, const llvm::Module &) const;
		void populateFromFile(const std::string &filename, const llvm::Module &);
		std::pair<int, int> annotate(const LengthInfo &info) const;
		std::map<llvm::Function const*, ValueSetToMaxIndexMap> maxIndexes;
	    std::map<llvm::Function const*, LengthValueSetMap> lengths;
	};
	
	char Annotator::ID;
	static const llvm::RegisterPass<Annotator> registration("annotator",
		"Determine whether and how to annotate each function",
		true, true);
	static llvm::cl::list<std::string>
		dependencyFileNames("annotator-dependency",
			llvm::cl::ZeroOrMore,
			llvm::cl::value_desc("filename"),
			llvm::cl::desc("Filename containing Annotator results for dependencies; use multiple times to read multiple files"));
	static llvm::cl::opt<std::string>
		outputFileName("annotator-output",
			llvm::cl::Optional,
			llvm::cl::value_desc("filename"),
			llvm::cl::desc("Filename to write results to"));
	static llvm::cl::opt<std::string>
	    testOutputName("test-annotator",
	        llvm::cl::Optional,
	        llvm::cl::value_desc("filename"),
	        llvm::cl::desc("Filename to write results to for regression tests"));
	static llvm::cl::opt<bool> Fast ("fast", 
	        llvm::cl::desc("Skip struct results for faster computation."));
}

#endif // !INCLUDE_NULL_ANNOTATOR_HH
