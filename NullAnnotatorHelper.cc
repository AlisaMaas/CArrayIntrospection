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
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>

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

static bool existsNonOptionalSentinelCheck(const FunctionResults *checks, const Value &value) {
	if (checks == nullptr)
		return false;
	return any_of(*checks | map_values,
		      [&](const ValueToBlockSet &entry) { return !entry.at(&value).second; });
}

static bool hasLoopWithSentinelCheck(const FunctionResults *checks, const Value &val) {
	if (checks == nullptr)
		return false;
	return any_of(*checks | map_values,
		      [&](const ValueToBlockSet &entry) { return !entry.at(&val).first.empty(); });
}


bool annotate(const Value &value, AnnotationMap &annotations) {
	const AnnotationMap::const_iterator found = annotations.find(&value);
	return found != annotations.end() && found->second == NULL_TERMINATED;
}


Answer getAnswer(const Value &value, const AnnotationMap &annotations) {
	const AnnotationMap::const_iterator found = annotations.find(&value);
	return found == annotations.end() ? DONT_CARE : found->second;
}

static pair<bool, bool> trackThroughCalls(CallInstSet &calls, const Value *value, AnnotationMap &annotations, 
	unordered_map<const Value *, string> &reasons) {
	// if we haven't yet continued, process evidence from callees.
	bool foundNonNullTerminated = false;
	bool nextPlease = false;
	bool changed = false;
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
			const Value &actual = *call.getArgOperand(argNo);
			if (!valueReachesValue(*value, actual)) continue;
			DEBUG(dbgs() << "Name of arg: " << value->getName() << "\n");
			DEBUG(dbgs() << "hey, it matches!\n");

			auto parameter = next(formals, argNo);
			if (parameter == calledFunction->getArgumentList().end() || argNo != parameter->getArgNo()) {
				continue;
			}
			DEBUG(dbgs() << "About to enter the switch\n");
			switch (getAnswer(*parameter, annotations)) {
			case NULL_TERMINATED:
				DEBUG(dbgs() << "Marking NULL_TERMINATED\n");
				annotations[value] = NULL_TERMINATED;
				reasons[value] = "Called " + calledFunction->getName().str() + ", marked as null terminated in this position";
				changed = true;
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
	return pair<bool, bool>(nextPlease, changed);
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

//should be called only once per module.
bool processLoops(const Module &module, const FunctionToValues &checkNullTerminated, 
	std::unordered_map<const llvm::Function *, FunctionResults> const &allSentinelChecks,
	AnnotationMap &annotations, unordered_map<const Value *, string> &reasons) {
	bool changed = false;
	for (const Function &func : module) {
		FunctionResults functionChecks = allSentinelChecks.at(&func);
		for (const Value *value : checkNullTerminated.at(&func)) {
			if (existsNonOptionalSentinelCheck(&functionChecks, *value)) {
				DEBUG(dbgs() << "\tFound a non-optional sentinel check in some loop!\n");
				annotations[value] = NULL_TERMINATED;
				reasons[value] = "Found a non-optional sentinel check in some loop of this function.";
				changed = true;
				continue;
			}
		}
	}
	return changed;
}

/**
*
* Preconditions: any dependencies for the file have already been read in and added
* to the annotation map, sentinel checks are filled in from previous analysis results,
* processLoops has already been called. 
*
* Postcondition: All possible null terminated information about the given values
* has been propagated - no more information can be obtained by iterating unless more
* information is added from another pass.
*
**/
bool iterateOverModule(const Module &module, const FunctionToValues &checkNullTerminated, 
	std::unordered_map<const llvm::Function *, FunctionResults> const &allSentinelChecks,
	unordered_map<const Function *, CallInstSet> &functionToCallSites, AnnotationMap &annotations,
	unordered_map<const Value *, string> &reasons) {
	
	//assume pre-populated with dependencies, and processLoops has already been called.
	bool globalChanged = false;
	bool changed;

	do {
		changed = false;
		for (const Function &func : module) {
			DEBUG(dbgs() << "About to get the map for this function\n");
			const FunctionResults &functionChecks = allSentinelChecks.at(&func);
			for (const Value *value : checkNullTerminated.at(&func)) {
				DEBUG(dbgs() << "\tConsidering " << value->getName() << "\n");
				Answer oldResult = getAnswer(*value, annotations);
				DEBUG(dbgs() << "\tOld result: " << oldResult << '\n');
				if (oldResult == NULL_TERMINATED)
					continue;
				// if we haven't yet continued, process evidence from callees.
				bool nextPlease = false;
				pair<bool, bool> callResponse = trackThroughCalls(functionToCallSites.at(&func), value, annotations, reasons);
				nextPlease = callResponse.first;
				changed |= callResponse.second;
				if (nextPlease) {
					continue;
				}
				// if we haven't yet marked NULL_TERMINATED, might be NON_NULL_TERMINATED
				if (hasLoopWithSentinelCheck(&functionChecks, *value)) {
					if (oldResult != NON_NULL_TERMINATED) {
						DEBUG(dbgs() << "Marking NOT_NULL_TERMINATED\n");
						annotations[value] = NON_NULL_TERMINATED;
						reasons[value] = "Has a loop with an optional sentinel check";
						changed = true;
						continue;
					}
				}
				// otherwise it stays as DONT_CARE for now.
			}
		}
		globalChanged |= changed;
	} while (changed);
	return globalChanged;
}