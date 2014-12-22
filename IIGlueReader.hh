#ifndef INCLUDE_IIGLUE_READER_HH
#define INCLUDE_IIGLUE_READER_HH

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/indirected.hpp>
#include <llvm/IR/Function.h>
#include <llvm/Pass.h>
#include <unordered_set>

namespace llvm {
	class Argument;
}


////////////////////////////////////////////////////////////////////////
//
//  read iiglue analysis output and tie it to LLVM data structures
//

class IIGlueReader : public llvm::ModulePass {

private:
	// utility functor for filtering
	class IsArray {
	public:
		IsArray(const IIGlueReader &);
		bool operator()(const llvm::Argument &) const;

	private:
		const IIGlueReader &iiglue;
	};

	// formal function arguments marked as arrays by iiglue
	std::unordered_set<const llvm::Argument *> arrays;

	// functions having at least one array as formal argument
	typedef std::unordered_set<const llvm::Function *> FunctionSet;
	FunctionSet atLeastOneArrayArg;

public:
	// standard LLVM pass interface
	IIGlueReader();
	static char ID;
	void getAnalysisUsage(llvm::AnalysisUsage &) const override final;
	bool runOnModule(llvm::Module &) override final;
	void print(llvm::raw_ostream &, const llvm::Module *) const;

	// convenience methods to access loaded iiglue annotations
	typedef boost::filtered_range<IsArray, const llvm::Function::ArgumentListType> ArrayArgumentsRange;
	typedef boost::indirected_range<const FunctionSet> ArrayReceiversRange;
	bool isArray(const llvm::Argument &) const;
	ArrayArgumentsRange arrayArguments(const llvm::Function &function) const;
	ArrayReceiversRange arrayReceivers() const;
};


////////////////////////////////////////////////////////////////////////


inline IIGlueReader::IsArray::IsArray(const IIGlueReader &iiglue)
	: iiglue(iiglue) {
}


inline bool IIGlueReader::IsArray::operator()(const llvm::Argument &argument) const {
	return iiglue.isArray(argument);
}


inline IIGlueReader::IIGlueReader()
	: ModulePass(ID) {
}


inline bool IIGlueReader::isArray(const llvm::Argument &argument) const {
	return arrays.count(&argument) != 0;
}


inline IIGlueReader::ArrayArgumentsRange IIGlueReader::arrayArguments(const llvm::Function &function) const {
	return { IsArray(*this), function.getArgumentList() };
}


inline IIGlueReader::ArrayReceiversRange IIGlueReader::arrayReceivers() const {
	return atLeastOneArrayArg | boost::adaptors::indirected;
}


#endif // !INCLUDE_IIGLUE_READER_HH
