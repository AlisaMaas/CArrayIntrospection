#define DEBUG_TYPE "annotator"

#include "Annotator.hh"
#include "AnnotatorHelper.hh"
#include "CheckGetElementPtrVisitor.hh"
#include "FindLengthChecks.hh"
#include "SRA/SymbolicRangeAnalysis.h"

#include <boost/property_tree/json_parser.hpp>
#include <boost/range/combine.hpp>
#include <fstream>
#include <llvm/Support/raw_os_ostream.h>

using namespace boost;
using namespace boost::adaptors;
using namespace boost::property_tree;
using namespace llvm;
using namespace std;

#if (1000 * LLVM_VERSION_MAJOR + LLVM_VERSION_MINOR) < 3007
typedef llvm::LoopInfo LoopInfoWrapperPass;
#endif	// before LLVM 3.7


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


inline Annotator::Annotator()
	: ModulePass(ID) {
}


llvm::LoopInfo& Annotator::runLoopInfo(llvm::Function &func) {
#if (1000 * LLVM_VERSION_MAJOR + LLVM_VERSION_MINOR) < 3007
	return getAnalysis<LoopInfoWrapperPass>(func);
#else  // LLVM 3.7 or later
	return getAnalysis<LoopInfoWrapperPass>(func).getLoopInfo();
#endif	// LLVM 3.7 or later
}
/*
    NO_LENGTH_VALUE,
	PARAMETER_LENGTH,
	FIXED_LENGTH,
	INCONSISTENT,
	SENTINEL_TERMINATED
*/


pair<int, int> Annotator::annotate(const LengthInfo &info) const {
    DEBUG(dbgs() << "Annotate switch statement about to start\n");
    switch (info.type) {
        case NOT_FIXED_LENGTH:
            DEBUG(dbgs() << "Not fixed length\n");
            return pair<int, int>(0, -2);
	    case NO_LENGTH_VALUE:
	        DEBUG(dbgs() << "No length value\n");
	        return pair<int, int>(0,0);
	    case INCONSISTENT:
	        DEBUG(dbgs() << "Inconsistent type\n");
	        return pair<int, int>(1, -1);
	    case SENTINEL_TERMINATED:
	        DEBUG(dbgs() << "Symbolic length value\n");
	        return pair<int, int>(2, info.length);
	    case PARAMETER_LENGTH:
	        DEBUG(dbgs() << "About to getSymbolicLength\n");
	        return pair<int, int>(6, (info.length == -1? info.getSymbolicLength(): info.length));
	    case FIXED_LENGTH:
	        DEBUG(dbgs() << "Fixed length value\n");
	        return pair<int, int>(7, info.length);
	    default:
	        DEBUG(dbgs() << "Type is unknown\n");
	        abort();
	        return pair<int, int>(-1, -1);
	}
}


pair<int, int> Annotator::annotate(const Value &value) const {
	return annotate(findAssociatedAnswer(&value, annotations));
}


pair<int, int> Annotator::annotate(const StructElement &element) const {
	const AnnotationMap::const_iterator found = annotations.find(structElements.at(element));
	if(found != annotations.end()) {
	    return annotate(found->second);
	}
	else {
	    return pair<int, int>(0, -1);
	}
}


void Annotator::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<FindStructElements>();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<SymbolicRangeAnalysis>();
	usage.addRequired<LoopInfoWrapperPass>();
	
}


void Annotator::populateFromFile(const string &filename, const Module &module) {
	DEBUG(dbgs() << "Top of populateFromFile\n");
	ptree root;
	read_json(filename, root);
	DEBUG(dbgs() << "Read in json\n");
	const ptree &libraryFunctions = root.get_child("library_functions");
	DEBUG(dbgs() << "Size of library functions is " << libraryFunctions.size() << "\n");
	for (const auto &framePair : libraryFunctions) {
		// find corresponding LLVM function object
		const auto &name = framePair.second.get_child("function_name").get_value<string>();
		const Function * const function = module.getFunction(name);
		if (!function) {
			errs() << "warning: found function " << name << " in iiglue results but not in bitcode\n";
			continue;
		}
		else 
		    DEBUG(dbgs() << "Found function " << name << "\n");
		const Function::ArgumentListType &arguments = function->getArgumentList();
		const ptree &arg_annotations = framePair.second.get_child("arguments");
		DEBUG(dbgs() << "there are " << arg_annotations.size() << " arguments\n");
		if (arguments.size() != arg_annotations.size()) {
			errs() << "Warning: Arity mismatch between function " << name
			       << " in the .json file provided: " << filename
			       << " and the one found in the bitcode. Skipping.\n";
			continue;
		}
		for (const auto &slot : boost::combine(arguments, arg_annotations)) {
		    DEBUG(dbgs() << "Got some arguments\n");
			const Argument &argument = slot.get<0>();
			const ptree &arg_annotation = slot.get<1>().second;
			DEBUG(dbgs() << "This argument has size " << slot.get<1>().second.size() << "\n");
			if (!argumentToValueSet.count(&argument)) {
				ValueSet *values = new set<const Value*>();
				values->insert(&argument);
				argumentToValueSet[&argument] = values;
			}
			try{
                if(!arg_annotation.get_child("sentinel").get_value<string>().empty()) {
                    DEBUG(dbgs() << "Length is not empty\n");
                    annotations[argumentToValueSet.at(&argument)] = LengthInfo(SENTINEL_TERMINATED, 0); 
                    errs() << "Added a sentinel terminated thing!\n";
                    //TODO: generalize for other types of sentinels later once we support that.
                }
			} catch(...) {
			    DEBUG(dbgs() << "Not a sentinel terminated argument\n");
			}
			try{
			    int parameterNo = arg_annotation.get_child("symbolic").get_value<int>();
			    errs() << name << " has child " << argument.getArgNo() << " with symbolic length of " << parameterNo << "\n";
			    annotations[argumentToValueSet.at(&argument)] = LengthInfo(PARAMETER_LENGTH, parameterNo);
			} catch (...) {
			    DEBUG(dbgs() << "Not a symbolic length argument\n");
			}
			try{
			    int fixedLen = arg_annotation.get_child("fixed").get_value<int>();
			    annotations[argumentToValueSet.at(&argument)] = LengthInfo(FIXED_LENGTH, fixedLen);
			} catch (...) {
			    DEBUG(dbgs() << "Not a fixed length argument\n");
			}
			try{
			    int other = arg_annotation.get_child("other").get_value<int>();
			    if (other == -1) {
			        annotations[argumentToValueSet.at(&argument)] = LengthInfo(INCONSISTENT, other);
			    }
			} catch(...) {
			    DEBUG(dbgs() << "Doesn't have an other field, at least not an int one.\n");
			}
		}
	}
}


void Annotator::dumpToFile(const string &filename, const Module &module) const {
	ofstream out(filename);
	out << "{\n\t\"library_functions\": [\n";
	bool first = true;
	for (const Function &function : module) {
		if (function.isDeclaration()) continue;
		if (!first) {
			out << ",\n";
		}
        first = false;
        out << "\t\t{\n\t\t\t\"arguments\": [";
        string depth = "\t\t\t\t\t";
        for (const Argument &arg : function.getArgumentList()) {
            if (&arg != function.getArgumentList().begin()) {
                out << ",";
            }
            out << "\n\t\t\t\t{\n";
            out << depth;
            out << "\"argument_name\": \"" << arg.getName().str() << "\"";
            out << ",\n";
            out << depth;
            DEBUG(dbgs() << "About to get the reasons set\n");
            auto reason = reasons.find(*argumentToValueSet.at(&arg));
            DEBUG(dbgs() << "Got reasons!\n");
            out << "\"argument_reason\": \"" << (reason == reasons.end() ? 
                "" : reason->second) + "\"";
            out << ",\n";
            out << depth;
            DEBUG(dbgs() << "About to get annotation for " << arg.getName().str() << "\n");
            pair<int, int> annotation = annotate(arg);
            DEBUG(dbgs() << "Got the annotation\n");
            switch(annotation.first) {
                case 0:
                case 1:
                    out << "\"other\": " << annotation.second;
                    break;
                case 2:
                    out << "\"sentinel\": ";
                    if (annotation.second == 0) {
                        out << "\"NUL\"";
                    }
                    else {
                        out << "\"" << annotation.second << "\"";
                    }
                    break;
                case 6:
                    out << "\"symbolic\": " << annotation.second;
                    break;
                case 7:
                    out << "\"fixed\": " << annotation.second;
                    break;
                default:
                    DEBUG(dbgs() << "Unknown annotation, aborting!\n");
                    abort();
            }
            out << "\n\t\t\t\t}";
        }
        DEBUG(dbgs() << "Got all the arguments for this function\n");
        out << "\n\t\t\t],\n\t\t\t\"function_name\":\"" << function.getName().str() << "\"";
        out << "\n\t\t}";
	}
	DEBUG(dbgs() << "Processed all the functions\n");
	out << "\n\t]\n}\n";
	out.close();
}


bool Annotator::runOnModule(Module &module) {
	DEBUG(dbgs() << "Populate dependencies\n");
    
	for (const string &dependency : dependencyFileNames) {
		populateFromFile(dependency, module);
	}
	
	DEBUG(dbgs() << "done populating dependencies\n");
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
    const FindStructElements &findElements = getAnalysis<FindStructElements>();
    structElements = findElements.getStructElements();
	unordered_map<const Function *, CallInstSet> allCallSites = collectFunctionCalls(module);
    FunctionToLoopInformation functionLoopInfo;
	FunctionToValueSets toCheck;
	ValueSetSet allValueSets;
	errs() << "About to get struct elements and run SRA over them\n";
	for (Function &func : module) {

		if ((func.isDeclaration())) continue;
		if (!Fast) {
		    DEBUG(dbgs() << "Putting in some struct elements\n");
            for (auto tuple : structElements) {
                toCheck[&func].insert(tuple.second);
                allValueSets.insert(tuple.second);
            }
		}
		for (const Argument &arg : iiglue.arrayArguments(func)) {
			if(argumentToValueSet.count(&arg)) {
				DEBUG(dbgs() << "Already got a valueset\n");
				toCheck[&func].insert(argumentToValueSet.at(&arg));
				allValueSets.insert(argumentToValueSet.at(&arg));
			}
			else {
				DEBUG(dbgs() << "Make a new valueset\n");
				set<const Value*> *values = new set<const Value*>();
				assert(values != nullptr);
				values->insert(&arg);
				argumentToValueSet[&arg] = values;
				toCheck[&func].insert(values);
				allValueSets.insert(values);
			}
		}
		DEBUG(dbgs() << "Analyzing " << func.getName() << "\n");
        const SymbolicRangeAnalysis &sra = getAnalysis<SymbolicRangeAnalysis>(func);
        DEBUG(dbgs() << "Acquired sra\n");
        CheckGetElementPtrVisitor visitor(maxIndexes[&func], sra, module, lengths[&func], allValueSets);
        for(BasicBlock &visitee :  func) {
            DEBUG(dbgs() << "Visiting a new basic block...\n");
            visitor.visit(visitee);
        }
        for (const ValueSet *set : visitor.notConstantBounded) {
            annotations[set] = LengthInfo(NOT_FIXED_LENGTH, -1);
        }
        for (const ValueSet *set : visitor.notParameterBounded) {
            annotations[set] = LengthInfo(NOT_FIXED_LENGTH, -1);
        }
	}
	errs() << "Done with SRA!\n";
	
	DEBUG(dbgs() << "Get the length checks\n");
    for (Function &func : module) {
        if ((func.isDeclaration())) continue;
        vector<LoopInformation> loopInfo;
		DEBUG(dbgs() << "Getting loop info\n");
		const LoopInfo &LI = runLoopInfo(func);
		DEBUG(dbgs() << "Got loop info\n");
		for(const Loop *loop : LI) {
		    LoopInformation info;
		    SmallVector<BasicBlock *, 4> exitingBlocks;
		    loop->getExitingBlocks(info.second.second);
		    info.second.first = loop->getBlocks();
	        info.first = loop->getHeader();
		    loopInfo.push_back(info);
		}
		functionLoopInfo[&func] = loopInfo;
		DEBUG(dbgs() << "Got 'em\n");
	}
	
	for (const Function &func : iiglue.arrayReceivers()) {
		if (!maxIndexes[&func].empty()) {
			for (pair<const ValueSet *, long int> fixedResult: maxIndexes[&func]) {
				annotations[fixedResult.first] = LengthInfo(FIXED_LENGTH, fixedResult.second);
			}
		}
		if (!lengths[&func].empty()) {
			for (pair<const ValueSet *, const ValueSet *> symbolicResult: lengths[&func]) {
				annotations[symbolicResult.first] = LengthInfo(PARAMETER_LENGTH, symbolicResult.second, -1);
				errs() << "FOUND PARAM_LENGTH!!!\n";

			}
		}

		DEBUG(dbgs() << "went through all the instructions and grabbed calls\n");
		DEBUG(dbgs() << "We found " << functionToCallSites[&func].size() << " calls in " << func.getName() << '\n');
	}
	
	DEBUG(dbgs() << "Finished going through array recievers\n");
	map<const Value *, const ValueSet*> valueToValueSet;
    for (const ValueSet *v : allValueSets) {
        if (!annotations.count(v)) {
            annotations[v] = LengthInfo(NO_LENGTH_VALUE, -1);
        }
        for (const Value *val : *v) {
            valueToValueSet[val] = v;
        }
    }
	
	DEBUG(dbgs() << "Iterate over the module\n");
	//const map<Function*, LoopInfo> &functionToLoopInfo)
	errs() << "About to start the main loop\n";
	iterateOverModule(module, toCheck, allCallSites, annotations, functionLoopInfo, reasons, Fast, valueToValueSet);
	errs() << "About to write everything to disk.\n";
	DEBUG(dbgs() << "Dump to a file\n");
	if (!outputFileName.empty())
		dumpToFile(outputFileName, module);
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


void Annotator::print(raw_ostream &sink, const Module *module) const {
    DEBUG(dbgs() << "About to print some things\n");
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	for (const Function &func : *module) {
		if (func.isDeclaration()) continue;
		for (const Argument &arg : iiglue.arrayArguments(func))
			switch(annotate(arg).first) {
			    case 2:
				    sink << func.getName() << " with argument " << arg.getArgNo()
				         << " should be annotated NULL_TERMINATED (" << (getAnswer(*argumentToValueSet.at(&arg), annotations)).toString()
				        << ")  because " << reasons.at(*argumentToValueSet.at(&arg)) << "\n";
				    break;
				case 0:
				    break;
				default:
				    sink << func.getName() << " with argument " << arg.getArgNo()
				        << " should be annotated " << (getAnswer(*argumentToValueSet.at(&arg), annotations)).toString() << ".\n";
				    break;
			}
	}
        for (auto element : structElements)
            switch(annotate(element.first).first) {
                case 2:
                    sink << str(&element.first)
                         << " should be annotated NULL_TERMINATED (" << (getAnswer(*element.second, annotations)).toString()
                         << ") because " << reasons.at(*element.second) << ".\n";
                    break;
                case 0:
                    break;
                default:
                    sink << str(&element.first) 
        		        << " should be annotated " << ((getAnswer(*element.second, annotations)).toString()) << ".\n";
                }
    DEBUG(dbgs() << "Finished printing things\n");
}
