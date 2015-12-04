#define DEBUG_TYPE "find-length"

#include "CallInstSet.hh"
#include "FindLengthChecks.hh"
#include "IIGlueReader.hh"
#include "ValueSetsReachingValue.hh"
//#include "ValueReachesValue.hh"

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
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_os_ostream.h>

#if (1000 * LLVM_VERSION_MAJOR + LLVM_VERSION_MINOR) >= 3005
#include <llvm/IR/InstIterator.h>
#else  // LLVM 3.4 or earlier
#include <llvm/Support/InstIterator.h>
#endif	// LLVM 3.4 or earlier

using namespace boost::adaptors;
using namespace boost::algorithm;
using namespace boost;
using namespace llvm;
using namespace std;

static unordered_map<const Function *, CallInstSet> collectFunctionCalls(const Module &module) {
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
static Value *stripSExtInst(Value *value) {
    while (SExtInst * SEI = dyn_cast<SExtInst>(value)) {
        value = SEI->getOperand(0);
    }
    if (PHINode *phi = dyn_cast<PHINode>(value)) {
        if (Value * v = phi->hasConstantValue()) {
          return v;
        }
    }
    return value;
}

const ValueSet *CheckGetElementPtrVisitor::getValueLength(Value *first, Value *second, const Value *basePointer) {
    const ValueSetSet reaching = valueSetsReachingValue(*first, valueSets);
    if (reaching.empty()) return nullptr;
    else if (reaching.size() == 1) {
        ConstantInt* c;
        if ((c = dyn_cast<ConstantInt>(second)) && c->isMinusOne()) {
            return *reaching.begin();
        }
        else return nullptr;
    }
    else {
        notConstantBounded.insert(getValueSetFromValue(basePointer, valueSets));
        notParameterBounded.insert(getValueSetFromValue(basePointer, valueSets));
        return nullptr;
    }
}
bool CheckGetElementPtrVisitor::matchAddPattern(Value *value, Value *basePointer) {
    DEBUG(dbgs() << "Looking at " << *value << " in matchAddPattern\n");
    value = stripSExtInst(value);
    DEBUG(dbgs() << "Looking at stripped " << *value << " in matchAddPattern\n");
    if (BinaryOperator * op = dyn_cast<BinaryOperator>(value)) {
        DEBUG(dbgs() << "Binary Operation detected!\n");
        if (op->getOpcode() == Instruction::Add) {
            DEBUG(dbgs() << "Yay it's an add!\n");
            Value *firstOperand = stripSExtInst(op->getOperand(0));
            Value *secondOperand = stripSExtInst(op->getOperand(1));
            DEBUG(dbgs() << "First operand: " << *firstOperand << "\n");
            DEBUG(dbgs() << "Second operand: " << *secondOperand << "\n");
            const ValueSet *length = getValueLength(firstOperand, secondOperand, basePointer);
            if (!length) length = getValueLength(secondOperand, firstOperand, basePointer);
            if (length) {
                DEBUG(dbgs() << "Hey, look, an argument length! \n");
                const ValueSet *valueSet = getValueSetFromValue(basePointer, valueSets);
                const ValueSet *old = lengths[valueSet];
                if (old == nullptr) {
                    lengths[valueSet] = length;
                    return true;
                }
                else if (old == length) {
                    return true;
                }
                else {
                    notParameterBounded.insert(valueSet);
                }
            }
                
        }
    }
    return false;
}

CheckGetElementPtrVisitor::CheckGetElementPtrVisitor(ValueSetToMaxIndexMap &map, 
const SymbolicRangeAnalysis &ra, Module &m, LengthValueSetMap &l, ValueSetSet &v ) 
: maxIndexes(map), lengths(l), rangeAnalysis(ra), valueSets(v), module(m){
    placeHolder = nullptr;
    functionsToCallsites = collectFunctionCalls(m);
}
CheckGetElementPtrVisitor::~CheckGetElementPtrVisitor() {
    DEBUG(dbgs() << "destructor\n");
    for (const ValueSet * v : notConstantBounded) {
        DEBUG(dbgs() << "Kicking out some constants\n");
        maxIndexes.erase(v);
    }
    DEBUG(dbgs() << "Done with constants\n");
    for (const ValueSet * v : notParameterBounded) {
        lengths.erase(v);        
    }
    DEBUG(dbgs() << "Finished with the delete\n");
    if (placeHolder != nullptr) delete placeHolder;
}

void CheckGetElementPtrVisitor::visitGetElementPtrInst(GetElementPtrInst& gepi) {
    //ignore all GEPs that don't lead to a memory access
    //unless that goes into a function call.
    //bool useless = true;
    for (const User *user : gepi.users()) {
        if (StoreInst::classof(user)) {
           // useless = false;
            break;
        }
        if (dyn_cast<LoadInst>(user)) {
            //if (load->getType() != load->getPointerOperand()->getType()) {
              //  useless = false;
                break;
            //}
        }
        if (GetElementPtrInst::classof(user)) {
            //useless = false;
            break;
        }
        //TODO: fix up.
        const CallInstSet calls = functionsToCallsites[gepi.getParent()->getParent()];

        /*for (const CallInst *call : calls) {
		    for (const unsigned argNo : irange(0u, call->getNumArgOperands())) {
                const Value *actual = call->getArgOperand(argNo);
                
                if (!valueReachesValue(*actual, gepi)) {
                    continue;
                }
                else {
                    useless = false;
                    break;
                }
		    }
        }*/
      return;
    }
    //if (useless) return;
    if (placeHolder != nullptr)
        delete placeHolder;
    placeHolder = BasicBlock::Create(module.getContext());
    DEBUG(dbgs() << "Top of visitor\n");
    Value *pointer = gepi.getPointerOperand();
    DEBUG(dbgs() << "Pointer operand obtained: " << *pointer << "\n");
    
    const ValueSet *valueSet = getValueSetFromValue(pointer, valueSets);
    DEBUG(dbgs() << "Got the valueSet\n");
   if (!valueSet) { //might be null if it doesn't correspond to anything interesting like an argument, or
    //if it doesn't correspond to something iiglue thinks is an array.
        DEBUG(dbgs() << "ValueSet is null. Here's what we know about this pointer: " << *pointer << "\n");
        return;
    }
    
    DEBUG(dbgs() << "GEPI: " << gepi << "\n");
    /*if (gepi.getType() != gepi.getPointerOperandType()) { //possibly detecting the struct access pattern.
        DEBUG(dbgs() << "Types don't match. We have " << *gepi.getType() << " and " << *gepi.getPointerOperandType() << "\n");
        if (dyn_cast<PointerType>(gepi.getPointerOperandType()) != nullptr) {
            PointerType *pointerType = dyn_cast<PointerType>(gepi.getPointerOperandType());
            DEBUG(dbgs() << "Got a pointer\n");
            Type *pointee = pointerType->getElementType();
            DEBUG(dbgs() << "Pointee type is " << *pointee << "\n");
            if (dyn_cast<ArrayType>(pointee)) {
                ArrayType *arrayType = dyn_cast<ArrayType>(pointee);
                DEBUG(dbgs() << "array type is " << *arrayType << "\n");
                maxIndexes[valueSet] = arrayType->getNumElements();
                return;
            }
            else if (gepi.getType()->getPointerTo() != pointerType)
                return; //don't mark it as bad because it's possible that we've got an array of structs.

        }
    }*/
    if (gepi.getNumIndices() != 1) {
        DEBUG(dbgs() << "Ignoring this one!\n");
        DEBUG(dbgs() << "It has " << gepi.getNumIndices() << " indices.\n");
        DEBUG(dbgs() << "Pointer is null? " << (valueSet? "no" : "yes") << "\n");
        //return; //in this case, we don't care.
        //need to do some thinking about higher number of indices, and make sure to have a 
        //consistent way of thinking about it.
        //should probably look at how it's usually documented.
    }
    Value *index = &*gepi.idx_begin()->get();
    DEBUG(dbgs() << "About to get the range for " << *index << "\n");
    DEBUG(dbgs() << "\tAddress is " << index << "\n");
    SAGERange r = rangeAnalysis.getState(index);
    DEBUG(dbgs() << "[" << r.getLower() << "," << r.getUpper() << "]\n");
    DEBUG(dbgs() << "Got it!\n");
    for (User * user : gepi.users()) {
        if (BitCastInst::classof(user)) {
            return; //in this case, we have no information, since they started casting.
        }
    }
    if (r.getUpper().isConstant()) { //check range-analysis
        DEBUG(dbgs() << "Range not unknown!\n");
        long int index = r.getUpper().getInteger(); 
        DEBUG(dbgs() << "index = " << index << "\n");
        if (index > maxIndexes[valueSet]) {
            DEBUG(dbgs() << "Yay range analysis! Adding to the map!\n");
            maxIndexes[valueSet] = index;
        }
    }
    else {
        DEBUG(dbgs() << "Not constant index\n");
        notConstantBounded.insert(valueSet);
        DEBUG(dbgs() << "Index in question = " << *index << "\n");
        DEBUG(dbgs() << "Is integer type? " << index->getType()->isIntegerTy() << "\n");
        Value *symbolicIndexInst = rangeAnalysis.getRangeValuesFor(index, IRBuilder<>(placeHolder)).second;
        DEBUG(dbgs() << "As reported by range analysis: " << *symbolicIndexInst << "\n");
        bool foundMatch = false;
        foundMatch = matchAddPattern(symbolicIndexInst, pointer);
        if (!foundMatch) {
            if (SelectInst *op = dyn_cast<SelectInst>(symbolicIndexInst)) {
                DEBUG(dbgs() << "Select Instruction!" << *op << "\n");
                foundMatch = matchAddPattern(op->getTrueValue(), pointer);
                if (!foundMatch) foundMatch = matchAddPattern(op->getFalseValue(), pointer);
            }
        
        }
        if (!foundMatch) notParameterBounded.insert(valueSet);
    }
    DEBUG(dbgs() << "Bottom of visitor\n");
}

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
		   const ValueSet *set = getValueSetFromValue(&arg, valueSets);
			if (constantMap.count(set))
				sink << "\tArgument " << arg.getName() << " has max index " << constantMap.at(set) << '\n';
            else if (parameterLengthMap.count(set))
				sink << "\tArgument " << arg.getName() << " has max index argument " << (*parameterLengthMap.at(set)->begin())->getName() << '\n';
			else if (iiglue.isArray(arg))
				sink << "\tArgument " << arg.getName() << " has unknown max index.\n";
		}
	}
	
}
