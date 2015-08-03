#define DEBUG_TYPE "null-annotator"
#include "NullAnnotatorHelper.hh"
#include "NullAnnotator.hh"

#include <boost/property_tree/json_parser.hpp>
#include <boost/range/combine.hpp>
#include <fstream>
#include <llvm/Support/raw_os_ostream.h>

using namespace boost;
using namespace boost::adaptors;
using namespace boost::property_tree;
using namespace llvm;
using namespace std;


inline NullAnnotator::NullAnnotator()
	: ModulePass(ID) {
}

llvm::LoopInfo& NullAnnotator::runLoopInfo(llvm::Function &func) {
            return getAnalysis<llvm::LoopInfo>(func);
}
/*
    NO_LENGTH_VALUE,
	PARAMETER_LENGTH,
	FIXED_LENGTH,
	INCONSISTENT,
	SENTINEL_TERMINATED
*/
pair<int, int> NullAnnotator::annotate(const LengthInfo &info) const {
    switch (info.type) {
	    case NO_LENGTH_VALUE:
	        return pair<int, int>(0,-1);
	    case INCONSISTENT:
	        return pair<int, int>(1, -1);
	    case SENTINEL_TERMINATED:
	        return pair<int, int>(2, info.length);
	    case PARAMETER_LENGTH:
	        return pair<int, int>(6, info.length);
	    case FIXED_LENGTH:
	        return pair<int, int>(7, info.length);
	    default:
	        abort();
	        return pair<int, int>(-1, -1);
	}
}
pair<int, int> NullAnnotator::annotate(const Value &value) const {
	return annotate(findAssociatedAnswer(&value, annotations));
}

pair<int, int> NullAnnotator::annotate(const StructElement &element) const {
	const AnnotationMap::const_iterator found = annotations.find(structElements.at(element));
	if(found != annotations.end()) {
	    return annotate(found->second);
	}
	else {
	    return pair<int, int>(0, -1);
	}
}

void NullAnnotator::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<FindLengthChecks>();
	usage.addRequired<FindStructElements>();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<LoopInfo>();
	
}

void NullAnnotator::populateFromFile(const string &filename, const Module &module) {
(void)filename;
(void)module;
	/*DEBUG(dbgs() << "Top of populateFromFile\n");
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
				ValueSet *values = new set<const Value*>();
				values->insert(&argument);
				argumentToValueSet[&argument] = values;
			}
			annotations[argumentToValueSet.at(&argument)] = annotation;
		}
	}*/
}


template<typename Detail> static
void dumpArgumentDetails(ostream &out, const Function::ArgumentListType &argumentList, const char key[], const Detail &detail) {
(void)out;
(void)argumentList;
(void)key;
(void)detail;
	/*out << "\t\t\t\"" << key << "\": [";
	for (const Argument &argument : argumentList) {
		if (&argument != argumentList.begin())
			out << ", ";
		out << detail(argument);
	}
	out << ']';*/
}


void NullAnnotator::dumpToFile(const string &filename, const IIGlueReader &iiglue, const Module &module) const {
(void)filename;
(void)iiglue;
(void)module;
	/*ofstream out(filename);
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
					    const auto reason = reasons.find(*argumentToValueSet.at(&arg));
					    return '\"' + (reason == reasons.end() ? "" : reason->second) + '\"';
				    }
			);

		out << "\n\t\t}";
	}
	out << "\n\t}\n}\n";*/
}


bool NullAnnotator::runOnModule(Module &module) {
	DEBUG(dbgs() << "Get the argumentToValueSet map for reuse\n");
    
	/*for (const string &dependency : dependencyFileNames) {
		populateFromFile(dependency, module);
	}*/
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
    const FindStructElements &findElements = getAnalysis<FindStructElements>();
    const FindLengthChecks &findLength = getAnalysis<FindLengthChecks>();
    (void) findLength;
    structElements = findElements.getStructElements();
	unordered_map<const Function *, CallInstSet> allCallSites = collectFunctionCalls(module);
    FunctionToLoopInformation functionLoopInfo;
	FunctionToValueSets toCheck;
	for (Function &func : module) {
		if ((func.isDeclaration())) continue;
		vector<LoopInformation> loopInfo;
		LoopInfo &LI = getAnalysis<LoopInfo>(func);
		for(const Loop *loop : LI) {
		    LoopInformation info;
		    SmallVector<BasicBlock *, 4> exitingBlocks;
		    loop->getExitingBlocks(info.second.second);
		    info.second.first = loop->getBlocks();
	        info.first = loop->getHeader();
		    loopInfo.push_back(info);
		}
		functionLoopInfo[&func] = loopInfo;
		if (!Fast) {
            for (auto tuple : structElements) {
                toCheck[&func].insert(tuple.second);
            }
		}
		for (const Argument &arg : iiglue.arrayArguments(func)) {
			if(argumentToValueSet.count(&arg)) {
				DEBUG(dbgs() << "Already got a valueset\n");
				toCheck[&func].insert(argumentToValueSet.at(&arg));
			}
			else {
				DEBUG(dbgs() << "Make a new valueset\n");
				set<const Value*> *values = new set<const Value*>();
				assert(values != nullptr);
				values->insert(&arg);
				argumentToValueSet[&arg] = values;
				toCheck[&func].insert(values);
			}
		}
		DEBUG(dbgs() << "Got 'em\n");
	}
	DEBUG(dbgs() << "Iterate over the module\n");
	//const map<Function*, LoopInfo> &functionToLoopInfo)
	iterateOverModule(module, toCheck, allCallSites, annotations, functionLoopInfo, reasons, Fast);
	DEBUG(dbgs() << "Dump to a file\n");
	if (!outputFileName.empty())
		dumpToFile(outputFileName, iiglue, module);
	DEBUG(dbgs() << "Done!\n");
    if (!testOutputName.empty()) {
        ofstream out(testOutputName);
        llvm::raw_os_ostream sink(out);	
        print(sink, &module);
        sink.flush();
        out.close();
    }
	return false;
}


void NullAnnotator::print(raw_ostream &sink, const Module *module) const {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	for (const Function &func : *module) {
		if (func.isDeclaration()) continue;
		for (const Argument &arg : iiglue.arrayArguments(func))
			if (annotate(arg).first == 2)
				sink << func.getName() << " with argument " << arg.getArgNo()
				     << " should be annotated NULL_TERMINATED (" << (getAnswer(*argumentToValueSet.at(&arg), annotations)).toString()
				     << ")  because " << reasons.at(*argumentToValueSet.at(&arg)) << "\n";
	}
        for (auto element : structElements)
            if (annotate(element.first).first == 2)
                sink << str(&element.first)
                     << " should be annotated NULL_TERMINATED (" << (getAnswer(*element.second, annotations)).toString()
                     << ") because " << reasons.at(*element.second) << ".\n";
    
}
