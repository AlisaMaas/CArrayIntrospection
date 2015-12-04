#define DEBUG_TYPE "find-length"

#include "CallInstSet.hh"
#include "CheckGetElementPtrVisitor.hh"
#include "FindLengthChecks.hh"
#include "IIGlueReader.hh"
#include "SRA/SymbolicRangeAnalysis.h"
#include "ValueSetsReachingValue.hh"

#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/foreach.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/iterator_range.hpp>
#include <fstream>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_os_ostream.h>

using namespace boost::algorithm;
using namespace boost;
using namespace llvm;
using namespace std;


static const RegisterPass<FindLengthChecks> registration("find-length",
		"Find arrays with a statically known constant or parameter-bounded length.",
		true, true);

char FindLengthChecks::ID;
static llvm::cl::opt<std::string>
    testOutputName("test-find-length",
        llvm::cl::Optional,
        llvm::cl::value_desc("filename"),
        llvm::cl::desc("Filename to write results to for regression tests"));

inline FindLengthChecks::FindLengthChecks()
	: ModulePass(ID) {
}

void FindLengthChecks::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<SymbolicRangeAnalysis>();
}


bool FindLengthChecks::runOnModule(Module &module) {
	DEBUG(dbgs() << "Top of runOnModule()\n");
	for (const Function &func : module) {
	    for (const Argument &arg : make_iterator_range(func.arg_begin(), func.arg_end())) {
	        ValueSet *set = new ValueSet();
	        set->insert(&arg);
	        valueSets.insert(set);
	    }
	
	}
	for (Function &func : module) {
	    if(func.isDeclaration()) continue;
		DEBUG(dbgs() << "Analyzing " << func.getName() << "\n");
		const SymbolicRangeAnalysis &ra = getAnalysis<SymbolicRangeAnalysis>(func);
		CheckGetElementPtrVisitor visitor(maxIndexes[&func], ra, module, lengths[&func], valueSets);
		for(BasicBlock &visitee :  func) {
			DEBUG(dbgs() << "Visiting a new basic block...\n");
			visitor.visit(visitee);
		}
	}
	if (!testOutputName.empty()) {
        ofstream out(testOutputName);
        llvm::raw_os_ostream sink(out);	
        print(sink, &module);
        sink.flush();
        out.close();
    }
	// read-only pass never changes anything
	return false;
}

void FindLengthChecks::print(raw_ostream &sink, const Module *module) const {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	for (const Function &func : *module) {
		const ValueSetToMaxIndexMap constantMap = maxIndexes.at(&func);
        const LengthValueSetMap parameterLengthMap = lengths.at(&func);
		sink << "Analyzing " << func.getName() << "\n";
		for (const Argument &arg : make_iterator_range(func.arg_begin(), func.arg_end())) {
		   const ValueSet *set = valueSets.getValueSetFromValue(&arg);
			if (constantMap.count(set))
				sink << "\tArgument " << arg.getName() << " has max index " << constantMap.at(set) << '\n';
            else if (parameterLengthMap.count(set))
				sink << "\tArgument " << arg.getName() << " has max index argument " << (*parameterLengthMap.at(set)->begin())->getName() << '\n';
			else if (iiglue.isArray(arg))
				sink << "\tArgument " << arg.getName() << " has unknown max index.\n";
		}
	}
	
}


const FunctionLengthResults FindLengthChecks::getResultsForFunction(const llvm::Function *func) const {
	const auto findConst = maxIndexes.find(func);
	const ValueSetToMaxIndexMap* first = (findConst == maxIndexes.end() ? nullptr : &findConst->second);

	const auto findParam = lengths.find(func);
	const LengthValueSetMap* second = (findParam == lengths.end() ? nullptr : &findParam->second);

	return {first, second};
}
