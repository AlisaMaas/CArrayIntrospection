#define DEBUG_TYPE "find-length"
#include "ArgumentsReachingValue.hh"
#include "FindLengthChecks.hh"
#include "FindSentinelHelper.hh"
#include "IIGlueReader.hh"

#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <SymbolicRangeAnalysis.h>
#include <fstream>



using namespace boost;
using namespace llvm;
using namespace std;

struct CheckGetElementPtrVisitor : public InstVisitor<CheckGetElementPtrVisitor> {
	const IIGlueReader *iiglue;
	ArgumentToMaxIndexMap &maxIndexes;
	LengthArgumentMap &lengthArguments;
	set<const Argument *> notConstantBounded;
    set<const Argument *> notParameterBounded;
	const SymbolicRangeAnalysis &rangeAnalysis;
	BasicBlock *placeHolder;
	CheckGetElementPtrVisitor(const IIGlueReader *r, ArgumentToMaxIndexMap &map, const SymbolicRangeAnalysis &ra, Module &m, LengthArgumentMap &lengths ) 
	: maxIndexes(map), lengthArguments(lengths), rangeAnalysis(ra) {
		placeHolder = BasicBlock::Create(m.getContext());
		iiglue = r;
	}
	~CheckGetElementPtrVisitor() {
        DEBUG(dbgs() << "destructor\n");
		for (const Argument * arg : notConstantBounded) {
            DEBUG(dbgs() << "Kicking out some constants\n");
            DEBUG(dbgs() << "About to erase " << *arg << "\n");
	  		maxIndexes.erase(arg);
	  	}
        DEBUG(dbgs() << "Done with constants\n");
        for (const Argument * arg : notParameterBounded) {
            lengthArguments.erase(arg);        
        }
        DEBUG(dbgs() << "Basic block holds " << placeHolder->getInstList().size()<< " things\n");
        delete placeHolder;
        DEBUG(dbgs() << "Finished with the delete\n");
		
	}
    Value *stripSExtInst(Value *value) {
        if (SExtInst * SEI = dyn_cast<SExtInst>(value)) {
            value = SEI->getOperand(0);
        }
        return value;
    }
    const Argument *getArgLength(Value *first, Value *second, const Argument *array) {
        const ArgumentSet reaching = argumentsReachingValue(*first);
        if (reaching.empty()) return nullptr;
        else if (reaching.size() == 1) {
            ConstantInt* c;
            if ((c = dyn_cast<ConstantInt>(second)) && c->isMinusOne()) {
                return *reaching.begin();
            }
            else return nullptr;
        }
        else {
            notParameterBounded.insert(array);
            return nullptr;
        }
    }
    bool matchAddPattern(Value *value, Argument *arg) {
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
                bool matches = false;
                const Argument *length = getArgLength(firstOperand, secondOperand, arg);
                if (!length) length = getArgLength(secondOperand, firstOperand, arg);
                if (length) {
                    DEBUG(dbgs() << "Hey, look, an argument length! " << *length << "\n");
                    const Argument *old = lengthArguments[arg];
                    if (old == nullptr) {
                        lengthArguments[arg] = length;
                        return true;
                    }
                    else if (old == length) {
                        return true;
                    }
                    else {
                        notParameterBounded.insert(arg);
                    }
                }
                    
            }
        }
        return false;
    }
	void visitGetElementPtrInst(GetElementPtrInst& gepi) {
		DEBUG(dbgs() << "Top of visitor\n");
		Value *pointer = gepi.getPointerOperand();
		Argument *arg = dyn_cast<Argument>(pointer);
		if (!arg) {
			DEBUG(dbgs() << "Argument is null. Here's what we know about this pointer: " << *pointer << "\n");
		}
		
		DEBUG(dbgs() << "GEPI: " << gepi << "\n");
		if (gepi.getNumIndices() != 1 || (arg? !(iiglue->isArray(*arg)) : true)) {
			DEBUG(dbgs() << "Ignoring this one!\n");
			DEBUG(dbgs() << "It has " << gepi.getNumIndices() << " indices.\n");
			DEBUG(dbgs() << "Pointer is null? " << (arg? "no" : "yes") << "\n");
			return; //in this case, we don't care.
			//need to do some thinking about higher number of indices, and make sure to have a 
			//consistent way of thinking about it.
			//should probably look at how it's usually documented.
		}
		if (gepi.getType() != gepi.getPointerOperandType()) { //possibly detecting the struct access pattern.
			//errs() << "Types don't match. We have " << *gepi.getType() << " and " << *gepi.getPointerOperandType() << "\n";
			return; //don't mark it as bad because it's possible that we've got an array of structs.
		}
		errs() << "GEPI: " << gepi << "\n";
		DEBUG(dbgs() << "About to get the range\n");
		SAGERange r = rangeAnalysis.getState(gepi.idx_begin()->get());
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
			if (index > maxIndexes[arg]) {
				DEBUG(dbgs() << "Yay range analysis! Adding to the map!\n");
				maxIndexes[arg] = index;
			}
		}
		else {
			DEBUG(dbgs() << "Not constant index\n");
            notConstantBounded.insert(arg);
			DEBUG(dbgs() << "Index in question = " << *gepi.idx_begin()->get() << "\n");
			DEBUG(dbgs() << "Is integer type? " << gepi.idx_begin()->get()->getType()->isIntegerTy() << "\n");
			Value *symbolicIndexInst = rangeAnalysis.getRangeValuesFor(gepi.idx_begin()->get(), IRBuilder<>(placeHolder)).second;
			DEBUG(dbgs() << "As reported by range analysis: " << *symbolicIndexInst << "\n");
            bool foundMatch = false;
            foundMatch = matchAddPattern(symbolicIndexInst, arg);
            if (!foundMatch) {
                if (SelectInst *op = dyn_cast<SelectInst>(symbolicIndexInst)) {
                    DEBUG(dbgs() << "Select Instruction!" << *op << "\n");
                    foundMatch = matchAddPattern(op->getTrueValue(), arg);
                    if (!foundMatch) foundMatch = matchAddPattern(op->getFalseValue(), arg);
                }
            
            }
            if (!foundMatch) notParameterBounded.insert(arg);
		}
		DEBUG(dbgs() << "Bottom of visitor\n");
	}
};

static const RegisterPass<FindLengthChecks> registration("find-length",
		"Find arrays with a statically known constant or parameter-bounded length.",
		true, true);

char FindLengthChecks::ID;


inline FindLengthChecks::FindLengthChecks()
	: ModulePass(ID) {
}

void FindLengthChecks::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<SymbolicRangeAnalysis >();
}


bool FindLengthChecks::runOnModule(Module &module) {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	const SymbolicRangeAnalysis &ra = getAnalysis<SymbolicRangeAnalysis>();
	DEBUG(dbgs() << "Top of runOnModule()\n");
	for (Function &func : module) {
		DEBUG(dbgs() << "Analyzing " << func.getName() << "\n");
		CheckGetElementPtrVisitor visitor(&iiglue, maxIndexes[&func], ra, module, lengthArguments[&func]);
		for(BasicBlock &visitee :  func) {
			DEBUG(dbgs() << "Visiting a new basic block...\n");
			visitor.visit(visitee);
		}
	}
	// read-only pass never changes anything
	return false;
}

void FindLengthChecks::print(raw_ostream &sink, const Module *module) const {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	for (const Function &func : *module) {
		const ArgumentToMaxIndexMap constantMap = maxIndexes.at(&func);
        const LengthArgumentMap parameterLengthMap = lengthArguments.at(&func);
		sink << "Analyzing " << func.getName() << "\n";
		for (const Argument &arg : make_iterator_range(func.arg_begin(), func.arg_end())) {
			if (constantMap.count(&arg))
				sink << "Argument " << arg.getName() << " has max index " << constantMap.at(&arg) << '\n';
            else if (parameterLengthMap.count(&arg))
				sink << "Argument " << arg.getName() << " has max index argument " << *parameterLengthMap.at(&arg) << '\n';
			else if (iiglue.isArray(arg))
				sink << "Argument " << arg.getName() << " has unknown max index.\n";
		}
	}
	
}
