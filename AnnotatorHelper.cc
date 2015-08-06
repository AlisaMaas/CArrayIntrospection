#define DEBUG_TYPE "annotator-helper"
#include "BacktrackPhiNodes.hh"
#include "FindSentinelHelper.hh"
#include "IIGlueReader.hh"
#include "LengthInfo.hh"
#include "AnnotatorHelper.hh"
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

LengthInfo mergeAnswers(LengthInfo first, LengthInfo second) {
    switch(first.type) {
        case NO_LENGTH_VALUE:
            return second;
        case INCONSISTENT:
            return first;
        case FIXED_LENGTH:
            switch(second.type) {
             case FIXED_LENGTH:
                if (first.length < second.length) {
                    return second;
                }
                else {
                    return first;
                }
            
            case NO_LENGTH_VALUE:
                return first;
            default:
                return second;
            }
        case SENTINEL_TERMINATED:
            switch(second.type) {
                case NO_LENGTH_VALUE:
                case FIXED_LENGTH:
                case SENTINEL_TERMINATED:
                    return first;
                case INCONSISTENT:
                case PARAMETER_LENGTH:
                    return second; //parameter-length beats sentinel-terminated
            }
        case PARAMETER_LENGTH:
            switch(second.type) {
                case NO_LENGTH_VALUE:
                case FIXED_LENGTH:
                case SENTINEL_TERMINATED:
                    return first;
                case PARAMETER_LENGTH:
                    if (first.length == second.length) {
                        if (first.length == -1) {
                            assert(first.symbolicLength != nullptr);
                            assert(second.symbolicLength != nullptr);
                            if (first.symbolicLength != second.symbolicLength) {
                                return LengthInfo(INCONSISTENT, -1);
                            }
                        }
                        return first;
                    }
                    else {
                        return LengthInfo(INCONSISTENT, -1);
                    }
                case INCONSISTENT:
                    return second;
            }
    }
    abort();
    return LengthInfo();
}

pair<int, int> annotate(LengthInfo &info) {
    switch (info.type) {
	    case NO_LENGTH_VALUE:
	        return pair<int, int>(0,-1);
	    case INCONSISTENT:
	        return pair<int, int>(1, -1);
	    case SENTINEL_TERMINATED:
	        return pair<int, int>(2, info.length);
	    case PARAMETER_LENGTH:
	        return pair<int, int>(6, (info.length == -1? info.getSymbolicLength() : info.length));
	    case FIXED_LENGTH:
	        return pair<int, int>(7, info.length);
	    default:
	        abort();
	        return pair<int, int>(-1, -1);
	}
}

pair<int, int> annotate(const ValueSet &value, AnnotationMap &annotations) {
	 AnnotationMap::iterator found = annotations.find(&value);
	if(found != annotations.end()) {
	    return annotate(found->second);
	}
	else {
	    return pair<int, int>(0, -1);
	}
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

LengthInfo findAssociatedAnswer(const Value *value, const AnnotationMap &annotations) {
	for (auto mapping : annotations) {
	    if (mapping.first == nullptr) {
	        continue;
	    }
		if (mapping.first->count(value)) {
			return mapping.second;
		}
	}
	return LengthInfo();
}

LengthInfo getAnswer(const ValueSet &value, const AnnotationMap &annotations) {
	const AnnotationMap::const_iterator found = annotations.find(&value);
	return found == annotations.end() ? LengthInfo() : found->second;
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
            LengthInfo old = annotations[valueSet];
            annotations[valueSet] = mergeAnswers(findAssociatedAnswer(pointer, annotations), old);
            if (old.type != annotations[valueSet].type || old.length != annotations[valueSet].length) {
                std::stringstream reason;
                reason << " pushed information from a store to ";
                reason << pointer->getName().str();
                reason << " from ";
                reason << value->getName().str();
                reasons[*valueSet] = reason.str();
                DEBUG(dbgs() << "Updating answer!\n");  
            }
            changed |= (old.type != annotations[valueSet].type || old.length != annotations[valueSet].length);
        }
    }
};


static pair<pair<LengthInfo, bool>, string> trackThroughCalls(CallInstSet &calls, const Value *value, AnnotationMap &annotations) {
	// if we haven't yet continued, process evidence from callees.
	//bool foundNonNullTerminated = false;
	bool nextPlease = false;
	std::stringstream reason;
	LengthInfo answer;
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
			auto parameter = next(formals, argNo);
			if (parameter == calledFunction->getArgumentList().end() || argNo != parameter->getArgNo()) {
				continue;
			}
			DEBUG(dbgs() << "About to enter the switch\n");
			LengthInfo formalAnswer = findAssociatedAnswer(parameter, annotations);
			switch (formalAnswer.type) {

	        case NO_LENGTH_VALUE:
	            break;
	        case INCONSISTENT:
	            answer = formalAnswer; //if treated inconsistently in a call, it's inconsistent
	            break;
	        case FIXED_LENGTH:
	            answer = formalAnswer;
	            reason << " found a call to " << call.getCalledFunction()->getName().str();
	            reason << " with fixed length of " << formalAnswer.length << " passing " << value->getName().str();
	            break;
	        case PARAMETER_LENGTH:
	            answer = formalAnswer;
	            reason << "found a call to " << call.getCalledFunction()->getName().str();
	            reason << " with parameter length of " << formalAnswer.length << " passing " << value->getName().str();
	            break;
			case SENTINEL_TERMINATED:
				answer = formalAnswer;
				reason << " found a call to " << call.getCalledFunction()->getName().str();
				reason << " passing " << value->getName().str();
				nextPlease = true;
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
	pair<LengthInfo, bool> partOne(answer, nextPlease);
	return pair<pair<LengthInfo, bool>, string> (partOne, reason.str());
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
//TODO: rename for consistency - this is just for finding sentinel checks in the loop
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
                    LengthInfo oldAnswer = getAnswer(*valueSet, annotations);
                    LengthInfo answer = oldAnswer;
                    for (const Value * value : *valueSet) {
                        //go through the loops.
                        LengthInfo valueAnswer;
                        //bool existsOptionalCheck = false;
                        if (firstTime) {
                            pair<bool, bool> result = processLoops(info.at(&func), value);
                            valueAnswer = result.second? LengthInfo(SENTINEL_TERMINATED, 0) : LengthInfo();
                            if (valueAnswer.type == SENTINEL_TERMINATED) {
                                std::stringstream reason;
                                reason << "found a non optional sentinel check for " << value->getName().str() << " in " << func.getName().str();
                                reasons[*valueSet] = reason.str();
                            }
                            //TODO: could do stuff here if we find an optional sentinel check, but it seems unwarranted.
                            //The thing we would do is make it inconsistent. But right now this doesn't give us any false positives
                            //So we don't bother.
                            //existsOptionalCheck = result.first;
                        }
                        // if we haven't yet continued, process evidence from callees.
                        DEBUG(dbgs() << "About to track through the calls\n");
                        pair<pair<LengthInfo, bool>, string> callResponse = trackThroughCalls(functionToCallSites.at(&func), value, annotations);
                        if (!callResponse.second.empty()) {
                            reasons[*valueSet] = callResponse.second;
                        }
                        DEBUG(dbgs() << "Finished going through the calls\n");
                        DEBUG(dbgs() << "Done finding sentinel checks\n");
                        DEBUG(dbgs() << "Result so far is " << valueAnswer.toString() << " \n");
                        // otherwise it stays as DONT_CARE for now.
                        valueAnswer = mergeAnswers(valueAnswer, callResponse.first.first);
                        DEBUG(dbgs() << "After merging, result is " << valueAnswer.toString() << "\n");
                        /*if (firstTime && valueAnswer == DONT_CARE) {
                            if (existsOptionalCheck) { //exists an optional sentinel check, and there's no evidence it is null terminated.
                                valueAnswer = NON_NULL_TERMINATED;
                            }
                        }*/
                        //DEBUG(dbgs() << "After looking for optional loops, result is " << valueAnswer << "\n");
                        answer = mergeAnswers(valueAnswer, answer);
                        DEBUG(dbgs() << "After merging, result is " << answer.toString() << "\n");
                    }
                    changed |= (answer.type != oldAnswer.type || answer.length != oldAnswer.length);
                    annotations[valueSet] = answer;
                }
            }
            firstTime = false;
            globalChanged |= changed;
        } while (changed);       
		DEBUG(dbgs() << "About to go get some stores\n");
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
        DEBUG(dbgs() << "Done with an iteration!\n");
	} while (changed);
	return globalChanged;
}