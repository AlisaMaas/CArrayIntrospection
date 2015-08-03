#define DEBUG_TYPE "length-annotator"
#include "ArgumentReachesValue.hh"
#include "ValueReachesValue.hh"
#include "BacktrackPhiNodes.hh"
#include "FindLengthChecks.hh"
#include "IIGlueReader.hh"
#include "LengthInfo.hh"

#include <boost/algorithm/cxx11/any_of.hpp>
#include <boost/foreach.hpp>
#include <boost/lambda/core.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/map.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/combine.hpp>
#include <boost/range/irange.hpp>
#include <boost/range/iterator_range.hpp>
#include <fstream>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_os_ostream.h>
#include <unordered_map>

#if (1000 * LLVM_VERSION_MAJOR + LLVM_VERSION_MINOR) >= 3005
#include <llvm/IR/InstIterator.h>
#else  // LLVM 3.4 or earlier
#include <llvm/Support/InstIterator.h>
#endif	// LLVM 3.4 or earlier

using namespace boost;
using namespace boost::adaptors;
using namespace boost::algorithm;
using namespace boost::property_tree;
using namespace llvm;
using namespace std;


namespace {
	class LengthAnnotator : public ModulePass {
	public:
		// standard LLVM pass interface
		LengthAnnotator();
		static char ID;
		void getAnalysisUsage(AnalysisUsage &) const final override;
		bool runOnModule(Module &) final override;
		void print(raw_ostream &, const Module *) const final override;

		// access to analysis results derived by this pass
		LengthInfo annotate(const Argument &) const;

	private:
		// map from function name and argument number to whether or not that argument gets annotated
		typedef unordered_map<const Argument *, LengthInfo> AnnotationMap;
		AnnotationMap annotations;
		unordered_map<const Argument *, string> reasons;
		typedef unordered_set<const CallInst *> CallInstSet;
		unordered_map<const Function *, CallInstSet> functionToCallSites;
		void dumpToFile(const string &filename, const IIGlueReader &, const Module &) const;
		//void populateFromFile(const string &filename, const Module &);
	};


	char LengthAnnotator::ID;
	static const RegisterPass<LengthAnnotator> registration("length-annotator",
		"Determine whether and how to annotate each function with length information.",
		true, true);
	static cl::list<string>
		dependencyFileNames("length-dependency",
			cl::ZeroOrMore,
			cl::value_desc("length-filename"),
			cl::desc("Filename containing Length results for dependencies; use multiple times to read multiple files"));
	static cl::opt<string>
		outputFileName("length-output",
			cl::Optional,
			cl::value_desc("filename"),
			cl::desc("Filename to write results to"));
	static llvm::cl::opt<std::string>
        testOutputName("test-length-annotator",
        llvm::cl::Optional,
        llvm::cl::value_desc("filename"),
        llvm::cl::desc("Filename to write results to for regression tests"));

}

inline LengthAnnotator::LengthAnnotator()
	: ModulePass(ID) {
}


LengthInfo LengthAnnotator::annotate(const Argument &arg) const {
	const AnnotationMap::const_iterator found = annotations.find(&arg);
	if (found == annotations.end()) return LengthInfo(NO_LENGTH_VALUE, -1);
	else return found->second;
}


void LengthAnnotator::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<FindLengthChecks>();
}


/*void LengthAnnotator::populateFromFile(const string &filename, const Module &module) {
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
			//TODO: Fix this whole thing up to work correctly again.
			//const LengthInfo annotation = static_cast<LengthInfo>(slot.get<1>().second.get_value<int>()); //TODO: fix
			//annotations[&argument] = annotation;
		}
	}
}*/


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


void LengthAnnotator::dumpToFile(const string &filename, const IIGlueReader &iiglue, const Module &module) const {
	ofstream out(filename);
	out << "{\n\t\"library_functions\": {\n";
	for (const Function &function : module) {

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
				    	if (annotate(arg).toString()[0] == 'p') {
				    		errs() << "Capital U detected\n";
				    		abort();
				    	}
					    return annotate(arg).toString(); //TODO: Fix
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
					    const auto reason = reasons.find(&arg);
					    return '\"' + (reason == reasons.end() ? "" : reason->second) + '\"';
				    }
			);

		out << "\n\t\t}";
	}
	out << "\n\t}\n}\n";
}


bool LengthAnnotator::runOnModule(Module &module) {
	/*for (const string &dependency : dependencyFileNames) {
		populateFromFile(dependency, module);
	}*/
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	const FindLengthChecks &findLength = getAnalysis<FindLengthChecks>();
	// collect calls in each function for repeated scanning later
	//TODO maybe pull this out into a util class since we use here and in NullAnnotator. Or could be pulled
	//into a common superclass.
	for (const Function &func : iiglue.arrayReceivers()) {
		const auto instructions =
			make_iterator_range(inst_begin(func), inst_end(func))
			| transformed([](const Instruction &inst) { return dyn_cast<CallInst>(&inst); })
			| filtered(boost::lambda::_1);
		functionToCallSites.emplace(&func, CallInstSet(instructions.begin(), instructions.end()));
		FunctionLengthResults results = findLength.getResultsForFunction(&func);
		if (results.first != nullptr) {
			for (pair<const Argument *, long int> fixedResult: *results.first) {
				annotations[fixedResult.first] = LengthInfo(FIXED_LENGTH, fixedResult.second);
			}
		}
		if (results.second != nullptr) {
			for (pair<const Argument *, const Argument *> fixedResult: *results.second) {
				annotations[fixedResult.first] = LengthInfo(PARAMETER_LENGTH, fixedResult.second->getArgNo());
				errs() << "FOUND PARAM_LENGTH!!!\n";

			}
		}
		for (const Argument &arg : iiglue.arrayArguments(func)) {
			if (!annotations.count(&arg)) {
				annotations[&arg] = LengthInfo(NO_LENGTH_VALUE, -1);
			}
		}
		DEBUG(dbgs() << "went through all the instructions and grabbed calls\n");
		DEBUG(dbgs() << "We found " << functionToCallSites[&func].size() << " calls in " << func.getName() << '\n');
	}
	bool changed;

	do {
		changed = false;
		for (const Function &func : iiglue.arrayReceivers()) {
			DEBUG(dbgs() << "About to get the map for this function\n");
			for (const Argument &arg : iiglue.arrayArguments(func)) {
				DEBUG(dbgs() << "\tConsidering " << arg.getArgNo() << "\n");
				LengthInfo oldResult = annotate(arg);
				DEBUG(dbgs() << "\tOld result: " << oldResult.toString() << '\n');
				if (oldResult.type == INCONSISTENT) break;
				bool nextArgumentPlease = false;
				for (const CallInst &call : functionToCallSites[&func] | indirected) {
					DEBUG(dbgs() << "About to iterate over the arguments to the call\n");
					//DEBUG(dbgs() << "Call: " << call.getName() << "\n");
					//DEBUG(dbgs() << "getCalledFunction name: " << call.getCalledFunction()->getName() << "\n");
					const auto calledFunction = call.getCalledFunction();
					if (calledFunction == nullptr)
						continue;
					const auto formals = calledFunction->getArgumentList().begin();
					DEBUG(dbgs() << "Got formals\n");
					for (const unsigned argNo : irange(0u, call.getNumArgOperands())) {
						DEBUG(dbgs() << "Starting iteration\n");
						const Value &actual = *call.getArgOperand(argNo);
						if (!argumentReachesValue(arg, actual)) continue;
						DEBUG(dbgs() << "Name of arg: " << arg.getName() << "\n");
						DEBUG(dbgs() << "hey, it matches!\n");

						auto parameter = next(formals, argNo);
						if (parameter == calledFunction->getArgumentList().end() || argNo != parameter->getArgNo()) {
							continue;
						}
						DEBUG(dbgs() << "About to enter the switch\n");
						LengthInfo calleeResult = annotate(*parameter); //TODO: Copy by reference??
						if(oldResult.type != calleeResult.type && oldResult.type != NO_LENGTH_VALUE &&
						calleeResult.type != NO_LENGTH_VALUE) { 
							//type is now inconsistent, needs reporting
							errs() << "Error: result went from " << oldResult.toString() << " to " << calleeResult.toString() << "\n";
							annotations[&arg] = LengthInfo(INCONSISTENT, -1);
							reasons[&arg] = "Result went from " + oldResult.toString() +
							" to " + calleeResult.toString();
							changed = true;
							break; //no point in looking through other calls if we've reached an inconsistent state.
						}
						switch (calleeResult.type) {
						case FIXED_LENGTH:
							DEBUG(dbgs() << "Saw FIXED_LENGTH\n");
							if (oldResult.type == FIXED_LENGTH) {
								DEBUG(dbgs() << "Merging two lengths\n");
								if (oldResult.length < calleeResult.length) {
									annotations[&arg] = calleeResult;
									reasons[&arg] = "Called " + calledFunction->getName().str() + 
									", marked as fixedLength(" + to_string(calleeResult.length) + ") in this position";
									changed = true;
								}
							}
							else { //must be NO_LENGTH_VALUE because of the check before the switch
								assert(oldResult.type == NO_LENGTH_VALUE);
								annotations[&arg] = calleeResult;
								reasons[&arg] = "Called " + calledFunction->getName().str() + 
								", marked as fixedLength(" + to_string(calleeResult.length) + ") in this position";
								changed = true;
							}
							//don't request the next argument because we may still have further fixed-lengths to merge.
							break;

						case PARAMETER_LENGTH:
							DEBUG(dbgs() << "Saw PARAMETER_LENGTH\n");
							if (oldResult.type == NO_LENGTH_VALUE) {
								annotations[&arg] = calleeResult;
								auto a = next(func.getArgumentList().begin(), (int)calleeResult.length);
								reasons[&arg] = "Called " + calledFunction->getName().str() + 
								", marked as parameter length of " + to_string(calleeResult.length) + " or " +
								a->getName().str() + " in this position";
								changed = true;
								break;
							}
							else if (oldResult.type == PARAMETER_LENGTH) {
							    const Value &actualLengthArg = *call.getArgOperand(calleeResult.length);
							    const Argument *formalLengthArg = nullptr;
							    long int i = 0;
							    for (const Argument &formalLengthArgCandidate : func.getArgumentList()) {
							        if (i == oldResult.length) {
							            formalLengthArg = &formalLengthArgCandidate;
							            break;
							        }
							        i++;
							    }
                                if (formalLengthArg != nullptr &&
                                    valueReachesValue(*formalLengthArg, actualLengthArg)) 
                                    break;
							}
							//if we get here, the state is inconsistent
                            annotations[&arg] = LengthInfo(INCONSISTENT, -1);
                            reasons[&arg] = "Result went from " + oldResult.toString() +
                             " to " + calleeResult.toString();
                             changed = true;
                             nextArgumentPlease = true; //can skip because now that it's inconsistent, it can't become
                             //consistent again.			
							break;

						case NO_LENGTH_VALUE:
							DEBUG(dbgs() << "Saw NO_LENGTH_VALUE\n");

							//this should just be safe to ignore, since we know nothing about the callee in this case.
							break;
                        case INCONSISTENT:
                            DEBUG(dbgs() << "Length made inconsistent!\n");
                            annotations[&arg] = LengthInfo(INCONSISTENT, -1);
                            reasons[&arg] = "Result went from " + oldResult.toString() +
                             " to " + calleeResult.toString();
                             changed = true;
                             nextArgumentPlease = true; //can skip because now that it's inconsistent, it can't become
                             //consistent again.
                             break;
						default:
							// should never happen!
							abort();
						}
					}

					if (nextArgumentPlease) {
						break;
					}
				}
			}
		}
	} while (changed);
	if (!outputFileName.empty())
		dumpToFile(outputFileName, iiglue, module);
	if (!testOutputName.empty()) {
        ofstream out(testOutputName);
        llvm::raw_os_ostream sink(out);	
        print(sink, &module);
        sink.flush();
        out.close();
    }
	return false;
}


void LengthAnnotator::print(raw_ostream &sink, const Module *module) const {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	for (const Function &func : *module) {
		for (const Argument &arg : iiglue.arrayArguments(func))
			//if (annotate(arg).type != NO_LENGTH_VALUE)
				sink << func.getName() << " with argument " << arg.getArgNo()
				     << " should be annotated " << (annotate(arg).toString()) //TODO: fix
				     << ".\n";
	}
}
