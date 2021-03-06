#define DEBUG_TYPE "annotator-helper"
#include "AnnotatorHelper.hh"
#include "BacktrackPhiNodes.hh"
#include "FindLengthLoops.hh"
#include "FindSentinelHelper.hh"
#include "IIGlueReader.hh"
#include "LengthInfo.hh"
#include "ValueReachesValue.hh"
#include "ValueSetSet.hh"

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
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <set>
#include <sstream>

#if (1000 * LLVM_VERSION_MAJOR + LLVM_VERSION_MINOR) >= 3005
#include <llvm/IR/InstIterator.h>
#else // LLVM 3.4 or earlier
#include <llvm/Support/InstIterator.h>
#endif // LLVM 3.4 or earlier

using namespace boost::adaptors;
using namespace boost::algorithm;
using namespace llvm;
using namespace std;


static LengthInfo mergeAnswers(LengthInfo first, LengthInfo second) {
	switch (first.type) {
	case NOT_FIXED_LENGTH:
		switch (second.type) {
		case FIXED_LENGTH:
			return first;
		default:
			return second;
		}
	case NO_LENGTH_VALUE:
		return second;
	case INCONSISTENT:
		return first;
	case FIXED_LENGTH:
		switch (second.type) {
		case FIXED_LENGTH:
			if (first.length < second.length) {
				return second;
			} else {
				return first;
			}

		case NO_LENGTH_VALUE:
			return first;
		default:
			return second;
		}
	case SENTINEL_TERMINATED:
		switch (second.type) {
		case NOT_FIXED_LENGTH:
		case NO_LENGTH_VALUE:
		case FIXED_LENGTH:
		case SENTINEL_TERMINATED:
			return first;
		case INCONSISTENT:
		case PARAMETER_LENGTH:
			return second; //parameter-length beats sentinel-terminated
		}
	case PARAMETER_LENGTH:
		switch (second.type) {
		case NOT_FIXED_LENGTH:
		case NO_LENGTH_VALUE:
		case FIXED_LENGTH:
		case SENTINEL_TERMINATED:
			return first;
		case PARAMETER_LENGTH:
			if (first.length == -1 && second.length != -1) {
				if (first.getSymbolicLength() != second.length) {
					return LengthInfo::inconsistent;
				} else {
					return second;
				}
			}
			if (second.length == -1 && first.length != -1) {
				if (second.getSymbolicLength() != first.length) {
					return LengthInfo::inconsistent;
				} else {
					return first;
				}
			}
			if (first.length == second.length) {
				if (first.length == -1) {
					assert(first.symbolicLength);
					assert(second.symbolicLength);
					if (first.symbolicLength != second.symbolicLength) {
						return LengthInfo::inconsistent;
					}
				}
				return first;
			} else if (first.getSymbolicLength() != second.getSymbolicLength()) {
				return LengthInfo::inconsistent;
			} else {
				return second;
			}
		case INCONSISTENT:
			return second;
		}
	}
	abort();
	return {};
}

static const shared_ptr<const ValueSet> findAssociatedValueSet(const Value *value, const map<const Value *, shared_ptr<const ValueSet>> &toCheck) {
	const auto found = toCheck.find(value);
	return found == toCheck.end() ? nullptr : found->second;
}

LengthInfo findAssociatedAnswer(const Value *value, const AnnotationMap &annotations) {
	for (auto mapping : annotations)
		if (mapping.first && mapping.first->count(value))
			return mapping.second;

	return {};
}

LengthInfo getAnswer(const shared_ptr<const ValueSet> &value, const AnnotationMap &annotations) {
	const auto found = annotations.find(value);
	return found == annotations.end() ? LengthInfo() : found->second;
}

struct ProcessStoresGEPVisitor : public InstVisitor<ProcessStoresGEPVisitor> {
	AnnotationMap &annotations;
	const map<const Value *, shared_ptr<const ValueSet>> &toCheck;
	map<shared_ptr<const ValueSet>, string> &reasons;
	bool changed;
	ProcessStoresGEPVisitor(AnnotationMap &a, const map<const Value *, shared_ptr<const ValueSet>> &v, map<shared_ptr<const ValueSet>, string> &r) : annotations(a), toCheck(v), reasons(r) {
		changed = false;
	}

	void visitStoreInst(StoreInst &store) {
		DEBUG(dbgs() << "Top of store instruction visitor\n");
		Value *pointer = store.getPointerOperand();
		Value *value = store.getValueOperand();
		const shared_ptr<const ValueSet> valueSet{findAssociatedValueSet(value, toCheck)};
		if (valueSet) {
			LengthInfo &current{annotations[valueSet]};
			const LengthInfo old{current};
			current = mergeAnswers(findAssociatedAnswer(pointer, annotations), old);
			if (old.type != current.type || old.length != current.length) {
				stringstream reason;
				reason << " pushed information from a store from ";
				reason << value->getName().str();
				reasons[valueSet] = reason.str();
				DEBUG(dbgs() << "Updating answer!\n");
			}
			changed |= (old.type != current.type || old.length != current.length);
		}
	}
};


static pair<LengthInfo, string> trackThroughCalls(CallInstSet &calls, const Value *value, AnnotationMap &annotations,
						  const Function &func, ValueSetSet &allValueSets) {
	// if we haven't yet continued, process evidence from callees.
	//bool foundNonNullTerminated = false;
	stringstream reason;
	LengthInfo answer = findAssociatedAnswer(value, annotations);
	//errs() << *value << "\n";
	if (answer.type == PARAMETER_LENGTH) {
		return {answer, "Preserved parameter length"};
	}
	for (const CallInst &call : calls | indirected) {
		DEBUG(dbgs() << "About to iterate over the arguments to the call\n");
		DEBUG(dbgs() << "Call: " << call.getName() << "\n");
		const Function *calledFunction = call.getCalledFunction();
		DEBUG(dbgs() << "getCalledFunction name: " << calledFunction << "\n");
		if (calledFunction == nullptr)
			continue;
		if (&func == calledFunction && answer.type == FIXED_LENGTH) {
			answer.type = NOT_FIXED_LENGTH;
		}
		//errs() << "getCalledFunction name: " << call.getCalledFunction()->getName() << "\n";
		const auto formals = calledFunction->getArgumentList().begin();
		DEBUG(dbgs() << "Got formals\n");
		for (const unsigned argNo : boost::irange(0u, call.getNumArgOperands())) {
			DEBUG(dbgs() << "Starting iteration\n");
			const Value *actual = call.getArgOperand(argNo);
			if (const LoadInst *load = dyn_cast<LoadInst>(actual)) {
				if (dyn_cast<GetElementPtrInst>(load->getPointerOperand()))
					actual = load->getPointerOperand();
			}
			if (const GetElementPtrInst *gepi = dyn_cast<GetElementPtrInst>(actual)) {
				if (gepi->getNumIndices() == 1 && gepi->hasAllZeroIndices())
					actual = gepi->getPointerOperand();
			}
			if ((answer.type != FIXED_LENGTH) && (!valueReachesValue(*value, *actual)))
				continue;
			else if (answer.type == FIXED_LENGTH && value != actual) {
				if (valueReachesValue(*value, *actual, true)) {
					answer.type = NOT_FIXED_LENGTH;
				}
				continue;
			}
			DEBUG(dbgs() << "match found!\n");
			auto parameter = next(formals, argNo);
			if (parameter == calledFunction->getArgumentList().end() || argNo != parameter->getArgNo()) {
				answer = mergeAnswers(answer, LengthInfo::notFixedLength);
				continue;
			}

			DEBUG(dbgs() << "About to enter the switch\n");
			LengthInfo formalAnswer = findAssociatedAnswer(parameter, annotations);
			//errs() << "Found formal arg to have " << formalAnswer.toString() << "\n";
			if (formalAnswer.type == PARAMETER_LENGTH) {
				int symbolicLen = formalAnswer.getSymbolicLength();
				formalAnswer = LengthInfo::notFixedLength;
				if (symbolicLen >= 0) {
					DEBUG(dbgs() << "Trying to figure out the symbolic length information\n");
					DEBUG(dbgs() << "In function " << func.getName() << " calling " << calledFunction->getName() << "\n");
					const ValueSetSet lengths{allValueSets.valueSetsReachingValue(*call.getArgOperand(symbolicLen))};
					if (lengths.size() == 1) {
						const auto &length = *lengths.begin();
						if (!length) {
							DEBUG(dbgs() << "Unable to find a length, at least we know it's not fixed length\n");
							formalAnswer = LengthInfo::notFixedLength;
						} else {
							DEBUG(dbgs() << "Marking parameter length\n");
							formalAnswer = LengthInfo::parameterLength(length);
						}
					} else {
						DEBUG(dbgs() << "We actually found " << lengths.size() << " things that can become this argument\n");
					}
				} else {
					DEBUG(dbgs() << "Symbolic length, but value is negative..oops\n");
				}
			}
			if (formalAnswer.type != FIXED_LENGTH && answer.type == FIXED_LENGTH) {
				answer = LengthInfo::notFixedLength;
			}
			answer = mergeAnswers(formalAnswer, answer);
			//errs() << "After merging, we decided that the answer is " << answer.toString() << "\n";
			if (answer.type == formalAnswer.type && formalAnswer.type != NO_LENGTH_VALUE && formalAnswer.type != NOT_FIXED_LENGTH && answer.length == formalAnswer.length) {
				reason.str(" ");
				reason << " found a call to " << call.getCalledFunction()->getName().str();
				// reason << " setting to " << answer.toString();
			}
		}

		if (answer.type == PARAMETER_LENGTH) break;
	}
	return {answer, reason.str()};
}

unordered_map<const Function *, CallInstSet> collectFunctionCalls(const Module &module) {
	unordered_map<const Function *, CallInstSet> functionToCallSites;
	// collect calls in each function for repeated scanning later
	for (const Function &func : module) {
		const auto instructions =
		    boost::make_iterator_range(inst_begin(func), inst_end(func)) | transformed([](const Instruction &inst) { return dyn_cast<CallInst>(&inst); }) | filtered(boost::lambda::_1);
		functionToCallSites.emplace(&func, CallInstSet(instructions.begin(), instructions.end()));
		DEBUG(dbgs() << "went through all the instructions and grabbed calls\n");
		DEBUG(dbgs() << "We found " << functionToCallSites[&func].size() << " calls in " << func.getName() << '\n');
	}
	return functionToCallSites;
}

//needs to be called per function.
//TODO: rename for consistency - this is just for finding sentinel checks in the loop
static LengthInfo processLoops(vector<LoopInformation> &LI, const Value *toCheck, const map<const Value *, shared_ptr<const ValueSet>> &valueToValueSet) {
	LengthInfo result;
	DEBUG(dbgs() << "Getting to look through loops now!\n");
	for (const LoopInformation loop : LI) {
		DEBUG(dbgs() << "Got a loop!\n");
		SentinelValueReport sentinelResponse = findSentinelChecks(loop, toCheck);
		if (!sentinelResponse.first.empty() && sentinelResponse.second) { //found an optional sentinel check in some loop for toCheck
			result = mergeAnswers(result, LengthInfo::notFixedLength);
		} //TODO: decide if I want to handle finding optional loops.
		if (!sentinelResponse.second) { //found a non-optional sentinel check in some loop for toCheck.
			result = mergeAnswers(result, LengthInfo::sentinelTerminated);
		}
		LengthValueReport lengthResponse = findLengthChecks(loop, toCheck);
		if (lengthResponse.size() == 1) {
			DEBUG(dbgs() << "Found a symbolic length check in a loop!\n");
			const Value *length = lengthResponse.begin()->first;
			pair<BlockSet, bool> lengthInfo = lengthResponse[length];
			if (!lengthInfo.second) { //found a non-optional length check in some loop for toCheck
				const auto &set = valueToValueSet.at(length);
				if (set) {
					DEBUG(dbgs() << "And it's non optional, too\n");
					result = mergeAnswers(result, LengthInfo::parameterLength(set));
				} else {
					result = mergeAnswers(result, LengthInfo::notFixedLength);
				}
			}
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
//TODO: fix name for checkNullTermianted.
bool iterateOverModule(Module &module, const FunctionToValueSets &checkNullTerminated,
		       unordered_map<const Function *, CallInstSet> &functionToCallSites, AnnotationMap &annotations,
		       FunctionToLoopInformation &info, map<shared_ptr<const ValueSet>, string> &reasons, bool fast,
		       map<const Value *, shared_ptr<const ValueSet>> &valueToValueSet) {

	//assume pre-populated with dependencies, and processLoops has already been called.
	bool globalChanged = false;
	bool changed;
	bool firstTime = true;

	ValueSetSet allValueSets;
	for (auto x : checkNullTerminated)
		allValueSets.insert(begin(x.second), end(x.second));

	do {
		do {
			changed = false;
			for (Function &func : module) {
				errs() << "Working on " << func.getName() << "\n";
				if ((func.isDeclaration())) continue;
				const auto found = checkNullTerminated.find(&func);
				if (found == checkNullTerminated.end()) continue;
				for (const auto &valueSet : found->second) {
					LengthInfo oldAnswer = getAnswer(valueSet, annotations);
					LengthInfo answer = oldAnswer;
					for (const Value *value : *valueSet) {
						//go through the loops.
						LengthInfo valueAnswer;
						//bool existsOptionalCheck = false;
						if (firstTime) {
							valueAnswer = processLoops(info.at(&func), value, valueToValueSet);
							if (valueAnswer.type == SENTINEL_TERMINATED) {
								stringstream reason;
								reason << "found a non optional sentinel check in " << func.getName().str();
								reasons[valueSet] = reason.str();
							} else if (valueAnswer.type == PARAMETER_LENGTH) {
								stringstream reason;
								reason << "found a non optional length check for " << value->getName().str() << " in " << func.getName().str();
								reasons[valueSet] = reason.str();
							}
							//TODO: could do stuff here if we find an optional sentinel check, but it seems unwarranted.
							//The thing we would do is make it inconsistent. But right now this doesn't give us any false positives
							//So we don't bother.
							//existsOptionalCheck = result.first;
						}
						// if we haven't yet continued, process evidence from callees.
						DEBUG(dbgs() << "About to track through the calls\n");
						//errs() << "Examining " << value->getName() << "\n";
						const auto callResponse = trackThroughCalls(functionToCallSites.at(&func),
											    value, annotations, func, allValueSets);
						if (!callResponse.second.empty()) {
							reasons[valueSet] = callResponse.second;
						}
						DEBUG(dbgs() << "Finished going through the calls\n");
						DEBUG(dbgs() << "Done finding sentinel checks\n");
						DEBUG(dbgs() << "Result so far is " << valueAnswer.toString() << " \n");
						// otherwise it stays as DONT_CARE for now.
						valueAnswer = mergeAnswers(valueAnswer, callResponse.first);
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
			errs() << "Done with an iteration\n";
		} while (changed);
		errs() << "About to get store information and push info through there...\n";
		DEBUG(dbgs() << "About to go get some stores\n");
		if (!fast) {
			for (Function &func : module) {
				DEBUG(dbgs() << "pushing information through stores.\n");
				ProcessStoresGEPVisitor visitor(annotations, valueToValueSet, reasons);
				for (BasicBlock &visitee : func) {
					visitor.visit(visitee);
				}
				changed |= visitor.changed;
			}
		}
		DEBUG(dbgs() << "Done with an iteration!\n");
	} while (changed);
	return globalChanged;
}
