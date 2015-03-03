#define DEBUG_TYPE "find-length"
#include "ArgumentsReachingValue.hh"
#include "FindLengthChecks.hh"
#include "IIGlueReader.hh"

#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <SymbolicRangeAnalysis.h>


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
	: maxIndexes(map), rangeAnalysis(ra), lengthArguments(lengths) {
		placeHolder = BasicBlock::Create(m.getContext());
		iiglue = r;
	}
	~CheckGetElementPtrVisitor() {
        errs() << "destructor\n";
		for (const Argument * arg : notConstantBounded) {
            errs() << "Kicking out some constants\n";
            errs() << "About to erase " << *arg << "\n";
	  		maxIndexes.erase(arg);
	  	}
        errs() << "Done with constants\n";
        for (const Argument * arg : notParameterBounded) {
            lengthArguments.erase(arg);        
        }
        errs() << "Basic block holds " << placeHolder->getInstList().size()<< " things\n";
        delete placeHolder;
        errs() << "Finished with the delete\n";
		
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
        errs() << "Looking at " << *value << " in matchAddPattern\n";
        value = stripSExtInst(value);
        errs() << "Looking at stripped " << *value << " in matchAddPattern\n";
        if (BinaryOperator * op = dyn_cast<BinaryOperator>(value)) {
            errs() << "Binary Operation detected!\n";
            if (op->getOpcode() == Instruction::Add) {
                errs() << "Yay it's an add!\n";
                Value *firstOperand = stripSExtInst(op->getOperand(0));
                Value *secondOperand = stripSExtInst(op->getOperand(1));
                errs() << "First operand: " << *firstOperand << "\n";
                errs() << "Second operand: " << *secondOperand << "\n";
                bool matches = false;
                const Argument *length = getArgLength(firstOperand, secondOperand, arg);
                if (!length) length = getArgLength(secondOperand, firstOperand, arg);
                if (length) {
                    errs() << "Hey, look, an argument length! " << *length << "\n";
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
		errs() << "Top of visitor\n";
		Value *pointer = gepi.getPointerOperand();
		Argument *arg = dyn_cast<Argument>(pointer);
		if (!arg) {
			errs() << "Argument is null. Here's what we know about this pointer: " << *pointer << "\n";
		}
		errs() << "GEPI: " << gepi << "\n";
		if (gepi.getNumIndices() != 1 || (arg? !(iiglue->isArray(*arg)) : true)) {
			errs() << "Ignoring this one!\n";
			errs() << "It has " << gepi.getNumIndices() << " indices.\n";
			errs() << "Pointer is null? " << (arg? "no" : "yes") << "\n";
			return; //in this case, we don't care.
			//need to do some thinking about higher number of indices, and make sure to have a 
			//consistent way of thinking about it.
			//should probably look at how it's usually documented.
		}
		errs() << "About to get the range\n";
		SAGERange r = rangeAnalysis.getState(gepi.idx_begin()->get());
        errs() << "[" << r.getLower() << "," << r.getUpper() << "]\n";
		errs() << "Got it!\n";
		if (r.getUpper().isConstant()) { //check range-analysis
			errs() << "Range not unknown!\n";
			long int index = r.getUpper().getInteger(); 
			errs() << "index = " << index << "\n";
			if (index > maxIndexes[arg]) {
				errs() << "Yay range analysis! Adding to the map!\n";
				maxIndexes[arg] = index;
			}
		}
		else {
			errs() << "Not constant index\n";
            notConstantBounded.insert(arg);
			errs() << "Index in question = " << *gepi.idx_begin()->get() << "\n";
			errs() << "Is integer type? " << gepi.idx_begin()->get()->getType()->isIntegerTy() << "\n";
			Value *symbolicIndexInst = rangeAnalysis.getRangeValuesFor(gepi.idx_begin()->get(), IRBuilder<>(placeHolder)).second;
			errs() << "As reported by range analysis: " << *symbolicIndexInst << "\n";
            bool foundMatch = false;
            foundMatch = matchAddPattern(symbolicIndexInst, arg);
            if (!foundMatch) {
                if (SelectInst *op = dyn_cast<SelectInst>(symbolicIndexInst)) {
                    errs() << "Select Instruction!" << *op << "\n";
                    foundMatch = matchAddPattern(op->getTrueValue(), arg);
                    if (!foundMatch) foundMatch = matchAddPattern(op->getFalseValue(), arg);
                }
            
            }
            if (!foundMatch) notParameterBounded.insert(arg);
		}
		errs() << "Bottom of visitor\n";
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
	errs() << "Top of runOnModule()\n";
	for (Function &func : module) {
		errs() << "Analyzing " << func.getName() << "\n";
		CheckGetElementPtrVisitor visitor(&iiglue, maxIndexes[&func], ra, module, lengthArguments[&func]);
		for(BasicBlock &visitee :  func) {
			errs() << "Visiting a new basic block...\n";
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
