#define DEBUG_TYPE "null-annotator-helper"
#include "Answer.hh"
#include "BacktrackPhiNodes.hh"
#include "FindSentinelHelper.hh"
#include "IIGlueReader.hh"
#include "NullAnnotatorHelper.hh"
#include "ValueReachesValue.hh"

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
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <set>
#include <sstream>

#if (1000 * LLVM_VERSION_MAJOR + LLVM_VERSION_MINOR) >= 3005
#include <llvm/IR/InstIterator.h>
#else  // LLVM 3.4 or earlier
#include <llvm/Support/InstIterator.h>
#endif	// LLVM 3.4 or earlier

using namespace boost;
using namespace boost::adaptors;
using namespace boost::algorithm;
using namespace llvm;
using namespace std;

Answer mergeAnswers(Answer first, Answer second) {
    switch(first) {
        case DONT_CARE:
            return second;
        case NON_NULL_TERMINATED:
            return first;
        case NULL_TERMINATED:
            switch(second) {
                case DONT_CARE:
                    return first;
                case NON_NULL_TERMINATED:
                    return second;
                case NULL_TERMINATED:
                    return first;
                default:
                    abort();
                    return DONT_CARE;
            }
        default:
            abort();
            return DONT_CARE;
    }
}

bool annotate(const ValueSet &value, const AnnotationMap &annotations) {
	const AnnotationMap::const_iterator found = annotations.find(&value);
	return found != annotations.end() && found->second == NULL_TERMINATED;
}

const ValueSet* findAssociatedValueSet(const Value *value, const FunctionToValueSets &toCheck) {
    for (auto tuple : toCheck) {
        for (const ValueSet *valueSet : tuple.second) {
            if (valueSet->count(value)) {
                return valueSet;
            }
        }
    }
    return nullptr;

}

Answer findAssociatedAnswer(const Value *value, const AnnotationMap &annotations) {
	for (auto mapping : annotations) {
		if (mapping.first->count(value)) {
			return mapping.second;
		}
	}
	return DONT_CARE;
}

Answer getAnswer(const ValueSet &value, const AnnotationMap &annotations) {
	const AnnotationMap::const_iterator found = annotations.find(&value);
	return found == annotations.end() ? DONT_CARE : found->second;
}

struct ProcessStoresGEPVisitor : public InstVisitor<ProcessStoresGEPVisitor> {
    AnnotationMap &annotations;
    const FunctionToValueSets &toCheck;
    map<const ValueSet, string> &reasons;
    bool changed;
    ProcessStoresGEPVisitor(AnnotationMap &a, const FunctionToValueSets &v, map<const ValueSet, string> &r) : 
    annotations(a), toCheck(v), reasons(r){
        changed = false;
    }

    void visitStoreInst(StoreInst& store) {
        DEBUG(dbgs() << "Top of store instruction visitor\n");
        Value* pointer = store.getPointerOperand();
        Value* value = store.getValueOperand();
        const ValueSet *valueSet = findAssociatedValueSet(value, toCheck);
        if (valueSet) {
            Answer old = annotations[valueSet];
            annotations[valueSet] = mergeAnswers(findAssociatedAnswer(pointer, annotations), old);
            if (old != annotations[valueSet]) {
                std::stringstream reason;
                reason << " pushed information from a store to ";
                reason << pointer->getName().str();
                reason << " from ";
                reason << value->getName().str();
                reasons[*valueSet] = reason.str();
                errs() << "Updating answer!\n";  
            }
            changed |= (old != annotations[valueSet]);
        }
    }
};


static pair<pair<Answer, bool>, string> trackThroughCalls(CallInstSet &calls, const Value *value, AnnotationMap &annotations) {
	// if we haven't yet continued, process evidence from callees.
	bool foundNonNullTerminated = false;
	bool nextPlease = false;
	std::stringstream reason;
	Answer answer = DONT_CARE;
	for (const CallInst &call : calls | indirected) {
		DEBUG(dbgs() << "About to iterate over the arguments to the call\n");
		DEBUG(dbgs() << "Call: " << call.getName() << "\n");
		DEBUG(dbgs() << "getCalledFunction name: " << call.getCalledFunction() << "\n");
		const auto calledFunction = call.getCalledFunction();
		if (calledFunction == nullptr)
			continue;
		const auto formals = calledFunction->getArgumentList().begin();
		DEBUG(dbgs() << "Got formals\n");
		for (const unsigned argNo : irange(0u, call.getNumArgOperands())) {
			DEBUG(dbgs() << "Starting iteration\n");
			const Value *actual = call.getArgOperand(argNo);
			if (const LoadInst *load = dyn_cast<LoadInst>(actual)) {
			    if (dyn_cast<GetElementPtrInst>(load->getPointerOperand()))
			        actual = load->getPointerOperand();
			}
			if (!valueReachesValue(*value, *actual)) continue;
			DEBUG(dbgs() << "match found!\n");
			auto parameter = std::next(formals, argNo);
			if (parameter == calledFunction->getArgumentList().end() || argNo != parameter->getArgNo()) {
				continue;
			}
			DEBUG(dbgs() << "About to enter the switch\n");
			switch (findAssociatedAnswer(parameter, annotations)) {
			case NULL_TERMINATED:
				answer = NULL_TERMINATED;
				reason << " found a call to " << call.getCalledFunction()->getName().str();
				reason << " passing " << value->getName().str();
				nextPlease = true;
				break;

			case NON_NULL_TERMINATED:
				// maybe set/check a flag for error reporting
				foundNonNullTerminated = true;
				break;

			case DONT_CARE:
				// maybe set/check a flag for error reporting
				if (foundNonNullTerminated) {
					DEBUG(dbgs() << "Found both DONT_CARE and NON_NULL_TERMINATED among callees.\n");
				}
				break;

			default:
				// should never happen!
				abort();
			}
		}

		if (nextPlease) {
			break;
		}
	}
	pair<Answer, bool> partOne(answer, nextPlease);
	return pair<pair<Answer, bool>, string> (partOne, reason.str());
}

unordered_map<const Function *, CallInstSet> collectFunctionCalls(const Module &module) {
	unordered_map<const Function *, CallInstSet> functionToCallSites;
	// collect calls in each function for repeated scanning later
	for (const Function &func : module) {
		const auto instructions =
			make_iterator_range(inst_begin(func), inst_end(func))
			| transformed([](const Instruction &inst) { return dyn_cast<CallInst>(&inst); })
			| filtered(boost::lambda::_1);
		functionToCallSites.emplace(&func, CallInstSet(instructions.begin(), instructions.end()));
		DEBUG(dbgs() << "went through all the instructions and grabbed calls\n");
		DEBUG(dbgs() << "We found " << functionToCallSites[&func].size() << " calls in " << func.getName() << '\n');
	}
	return functionToCallSites;
}

//needs to be called per function.
static pair<bool, bool> processLoops(vector<LoopInformation> &LI, const Value* toCheck) {
	pair<bool, bool> result(false, false);
	DEBUG(dbgs() << "Getting to look through loops now!\n");
	for (const LoopInformation loop: LI) {
	    DEBUG(dbgs() << "Got a loop!\n");
		ValueReport response = findSentinelChecks(loop, toCheck);
		if (!response.first.empty() && response.second) { //found an optional sentinel check in some loop for toCheck
		    result.first = true;
		}
		else if (!response.second) { //found a non-optional sentinel check in some loop for toCheck.
		    result.second = true;
		}
	}
	return result;
}

/**
*
* Preconditions: any dependencies for the file have already been read in and added
* to the annotation map. 
*
* Postcondition: All possible null terminated information about the given values
* has been propagated - no more information can be obtained by iterating unless more
* information is added from another pass.
*
**/
bool iterateOverModule(Module &module, const FunctionToValueSets &checkNullTerminated, 
	unordered_map<const Function *, CallInstSet> &functionToCallSites, AnnotationMap &annotations,
	FunctionToLoopInformation &info, map<const ValueSet, string> &reasons, bool fast) {
	
	//assume pre-populated with dependencies, and processLoops has already been called.
	bool globalChanged = false;
	bool changed;
    bool firstTime = true;
	do {
	    do {
            changed = false;
            for (Function &func : module) {
                DEBUG(dbgs() << "Working on " << func.getName() << "\n");
                if ((func.isDeclaration())) continue;
                if (!checkNullTerminated.count(&func)) continue;
                for (const ValueSet *valueSet : checkNullTerminated.at(&func)) {
                    Answer oldAnswer = getAnswer(*valueSet, annotations);
                    Answer answer = oldAnswer;
                    for (const Value * value : *valueSet) {
                        //go through the loops.
                        Answer valueAnswer = DONT_CARE;
                        //bool existsOptionalCheck = false;
                        if (firstTime) {
                            pair<bool, bool> result = processLoops(info.at(&func), value);
                            valueAnswer = result.second? NULL_TERMINATED : DONT_CARE;
                            if (valueAnswer == NULL_TERMINATED) {
                                std::stringstream reason;
                                reason << "found a non optional sentinel check for " << value->getName().str() << " in " << func.getName().str();
                                reasons[*valueSet] = reason.str();
                            }
                            //existsOptionalCheck = result.first;
                        }
                        // if we haven't yet continued, process evidence from callees.
                        DEBUG(dbgs() << "About to track through the calls\n");
                        pair<pair<Answer, bool>, string> callResponse = trackThroughCalls(functionToCallSites.at(&func), value, annotations);
                        if (!callResponse.second.empty()) {
                            reasons[*valueSet] = callResponse.second;
                        }
                        DEBUG(dbgs() << "Finished going through the calls\n");
                        DEBUG(dbgs() << "Done finding sentinel checks\n");
                        DEBUG(dbgs() << "Result so far is " << valueAnswer << " \n");
                        // otherwise it stays as DONT_CARE for now.
                        valueAnswer = mergeAnswers(valueAnswer, callResponse.first.first);
                        DEBUG(dbgs() << "After merging, result is " << valueAnswer << "\n");
                        /*if (firstTime && valueAnswer == DONT_CARE) {
                            if (existsOptionalCheck) { //exists an optional sentinel check, and there's no evidence it is null terminated.
                                valueAnswer = NON_NULL_TERMINATED;
                            }
                        }*/
                        DEBUG(dbgs() << "After looking for optional loops, result is " << valueAnswer << "\n");
                        answer = mergeAnswers(valueAnswer, answer);
                        DEBUG(dbgs() << "After merging, result is " << answer << "\n");
                    }
                    changed |= answer != oldAnswer;
                    annotations[valueSet] = answer;
                }
            }
            firstTime = false;
            globalChanged |= changed;
        } while (changed);       
		errs() << "About to go get some stores\n";
		if (!fast) {
            for (Function &func : module) {
                DEBUG(dbgs() << "pushing information through stores.\n");
                ProcessStoresGEPVisitor visitor(annotations, checkNullTerminated, reasons);
                for(BasicBlock &visitee :  func) {
                    visitor.visit(visitee);
                }
                changed |= visitor.changed;
            }
        }
        errs() << "Done with an iteration!\n";
	} while (changed);
	return globalChanged;
}
