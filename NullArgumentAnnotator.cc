#define DEBUG_TYPE "null-argument-annotator"
#include "Answer.hh"
#include "FindArraySentinels.hh"
#include "IIGlueReader.hh"
#include "NullAnnotatorHelper.hh"

#include <boost/property_tree/json_parser.hpp>
#include <boost/range/combine.hpp>
#include <fstream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

using namespace boost;
using namespace boost::adaptors;
using namespace boost::property_tree;
using namespace llvm;
using namespace std;


namespace {
	class NullArgumentAnnotator : public ModulePass {
	public:
		// standard LLVM pass interface
		NullArgumentAnnotator();
		static char ID;
		void getAnalysisUsage(AnalysisUsage &) const final override;
		bool runOnModule(Module &) final override;
		void print(raw_ostream &, const Module *) const final override;

		// access to analysis results derived by this pass
		bool annotate(const Argument &) const;

	private:
		// map from function name and argument number to whether or not that argument gets annotated
		AnnotationMap annotations;
		unordered_map<const Argument *, const ValueSet*> argumentToValueSet;
		unordered_map<const ValueSet*, string> reasons;
		typedef unordered_set<const CallInst *> CallInstSet;
		unordered_map<const Function *, CallInstSet> functionToCallSites;
		void dumpToFile(const string &filename, const IIGlueReader &, const Module &) const;
		void populateFromFile(const string &filename, const Module &);
	};


	char NullArgumentAnnotator::ID;
	static const RegisterPass<NullArgumentAnnotator> registration("null-argument-annotator",
		"Determine whether and how to annotate each function with the null-terminated annotation",
		true, true);
	static cl::list<string>
		dependencyFileNames("dependency",
			cl::ZeroOrMore,
			cl::value_desc("filename"),
			cl::desc("Filename containing NullAnnotator results for dependencies; use multiple times to read multiple files"));
	static cl::opt<string>
		outputFileName("output",
			cl::Optional,
			cl::value_desc("filename"),
			cl::desc("Filename to write results to"));
}

inline NullArgumentAnnotator::NullArgumentAnnotator()
	: ModulePass(ID) {
}


bool NullArgumentAnnotator::annotate(const Argument &arg) const {
	const AnnotationMap::const_iterator found = annotations.find(argumentToValueSet.at(&arg));
	return found != annotations.end() && found->second == NULL_TERMINATED;
}


void NullArgumentAnnotator::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<FindArraySentinels>();
}

void NullArgumentAnnotator::populateFromFile(const string &filename, const Module &module) {
	DEBUG(dbgs() << "Top of populateFromFile\n");
	ptree root;
	read_json(filename, root);
	const ptree &libraryFunctions = root.get_child("library_functions");
	for (const auto &framePair : libraryFunctions) {
		// find corresponding LLVM function object
		const string &name = framePair.first;
		const Function * const function = module.getFunction(name);
		if (!function) {
			errs() << "warning: found function " << name << " in iiglue results but not in bitcode\n";
			continue;
		}

		const Function::ArgumentListType &arguments = function->getArgumentList();
		const ptree &arg_annotations = framePair.second.get_child("argument_annotations");
		if (arguments.size() != arg_annotations.size()) {
			errs() << "Warning: Arity mismatch between function " << name
			       << " in the .json file provided: " << filename
			       << " and the one found in the bitcode. Skipping.\n";
			continue;
		}
		for (const auto &slot : boost::combine(arguments, arg_annotations)) {
			const Argument &argument = slot.get<0>();
			const Answer annotation = static_cast<Answer>(slot.get<1>().second.get_value<int>());
			if (!argumentToValueSet.count(&argument)) {
				ValueSet *values = new unordered_set<const Value*>();
				values->insert(&argument);
				argumentToValueSet[&argument] = values;
			}
			annotations[argumentToValueSet.at(&argument)] = annotation;
		}
	}
}


template<typename Detail> static
void dumpArgumentDetails(ostream &out, const Function::ArgumentListType &argumentList, const char key[], const Detail &detail) {
	out << "\t\t\t\"" << key << "\": [";
	for (const Argument &argument : argumentList) {
		if (&argument != argumentList.begin())
			out << ", ";
		out << detail(argument);
	}
	out << ']';
}


void NullArgumentAnnotator::dumpToFile(const string &filename, const IIGlueReader &iiglue, const Module &module) const {
	ofstream out(filename);
	out << "{\n\t\"library_functions\": {\n";
	for (const Function &function : module) {
		if (function.isDeclaration()) continue;
		if (&function != module.begin())
			out << ",\n";

		out << "\t\t\"" << function.getName().str() << "\": {\n";
		const Function::ArgumentListType &argumentList = function.getArgumentList();

		dumpArgumentDetails(out, argumentList, "argument_names",
				    [](const Argument &arg) {
					    return '\"' + arg.getName().str() + '\"';
				    }
			);
		out << ",\n";

		dumpArgumentDetails(out, argumentList, "argument_annotations",
				    [&](const Argument &arg) {
					    return getAnswer(*argumentToValueSet.at(&arg), annotations);
				    }
			);
		out << ",\n";

		dumpArgumentDetails(out, argumentList, "args_array_receivers",
				    [&](const Argument &arg) {
					    return iiglue.isArray(arg);
				    }
			);
		out << ",\n";

		dumpArgumentDetails(out, argumentList, "argument_reasons",
				    [&](const Argument &arg) {
					    const auto reason = reasons.find(argumentToValueSet.at(&arg));
					    return '\"' + (reason == reasons.end() ? "" : reason->second) + '\"';
				    }
			);

		out << "\n\t\t}";
	}
	out << "\n\t}\n}\n";
}


bool NullArgumentAnnotator::runOnModule(Module &module) {
	const FindArraySentinels &findArraySentinels = getAnalysis<FindArraySentinels>();
	DEBUG(dbgs() << "Get the argumentToValueSet map for reuse\n");
	argumentToValueSet = findArraySentinels.getArgumentToValueSet();

	for (const string &dependency : dependencyFileNames) {
		populateFromFile(dependency, module);
	}
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();

	unordered_map<const Function *, CallInstSet> allCallSites = collectFunctionCalls(module);

	FunctionToValueSets toCheck;
	unordered_map<const Function *, FunctionResults> allSentinelChecks;
	for (const Function &func : iiglue.arrayReceivers()) {
		if ((func.isDeclaration())) continue;
		DEBUG(dbgs() << "About to get the findSentinels results\n");
		allSentinelChecks[&func] = *findArraySentinels.getResultsForFunction(&func);
		DEBUG(dbgs() << "Collect up the valuesets for " << func.getName() << "\n");
		for (const Argument &arg : iiglue.arrayArguments(func)) {
			if(argumentToValueSet.count(&arg)) {
				DEBUG(dbgs() << "Already got a valueset\n");
				toCheck[&func].insert(argumentToValueSet.at(&arg));
			}
			else {
				DEBUG(dbgs() << "Make a new valueset\n");
				unordered_set<const Value*> *values = new unordered_set<const Value*>();
				assert(values != nullptr);
				values->insert(&arg);
				argumentToValueSet[&arg] = values;
				toCheck[&func].insert(values);
			}
		}
		DEBUG(dbgs() << "Got 'em\n");
	}
	DEBUG(dbgs() << "Process loops\n");
	processLoops(module, toCheck, allSentinelChecks, annotations, reasons);
	DEBUG(dbgs() << "Iterate over the module\n");
	iterateOverModule(module, toCheck, allSentinelChecks, allCallSites, annotations, reasons);
	DEBUG(dbgs() << "Dump to a file\n");
	if (!outputFileName.empty())
		dumpToFile(outputFileName, iiglue, module);
	DEBUG(dbgs() << "Done!\n");
	return false;
}


void NullArgumentAnnotator::print(raw_ostream &sink, const Module *module) const {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	for (const Function &func : *module) {
		if (func.isDeclaration()) continue;
		for (const Argument &arg : iiglue.arrayArguments(func))
			if (annotate(arg))
				sink << func.getName() << " with argument " << arg.getArgNo()
				     << " should be annotated NULL_TERMINATED (" << (getAnswer(*argumentToValueSet.at(&arg), annotations))
				     << ").\n";
	}
}
