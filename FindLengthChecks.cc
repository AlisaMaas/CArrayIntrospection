#include "FindLengthChecks.hh"
#include "IIGlueReader.hh"
#include "UpperBoundIndexing.hh"

#include <llvm/Analysis/LoopPass.h>
#include <llvm/InstVisitor.h>
#include <llvm/IR/Module.h>

using namespace boost;
using namespace llvm;
using namespace std;

struct CheckGetElementPtrVisitor : public InstVisitor<CheckGetElementPtrVisitor> {
	const IIGlueReader *iiglue;
	ArgumentToMaxIndexMap &maxIndexes;
	CheckGetElementPtrVisitor(const IIGlueReader *r, ArgumentToMaxIndexMap &map) : maxIndexes(map) {
		iiglue = r;
	}
	void visitGetElementPtrInst(GetElementPtrInst& gepi) {
		Value *pointer = gepi.getPointerOperand();
		Argument *arg = dyn_cast<Argument>(pointer);
		errs() << "GEPI: " << gepi << "\n";
		if (gepi.getNumIndices() != 1 || (arg? !(iiglue->isArray(*arg)) : true)) {
			return; //in this case, we don't care.
		}
		ConstantInt *constant;
		if ((constant = dyn_cast<ConstantInt>(gepi.idx_begin()->get()))) {
			int index = constant->getSExtValue();
			if(index > maxIndexes[arg]) {
				maxIndexes[arg] = index;
				errs() << "Adding to map\n";
			}
		}
		
		else {
			errs() << "FLAG HERE - not constant bounded\n";
		}
	}
};

static const RegisterPass<FindLengthChecks> registration("find-length",
		"Find loops that have a fixed number of iterations that also index into arrays.",
		true, true);

char FindLengthChecks::ID;


inline FindLengthChecks::FindLengthChecks()
	: ModulePass(ID) {
}

void FindLengthChecks::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	//usage.setPreservesAll();
	//usage.addRequired<LoopInfo>();
	usage.addRequired<IIGlueReader>();
	usage.addRequired<UpperBoundIndexing>();
	//usage.addRequired<ScalarEvolution>();
}


bool FindLengthChecks::runOnModule(Module &module) {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	errs() << "Top of runOnModule()\n";
	for (Function &func : module) {
		errs() << "Analyzing " << func.getName() << "\n";
		CheckGetElementPtrVisitor visitor(&iiglue, maxIndexes[&func]);
		for(BasicBlock &visitee :  func) {
			visitor.visit(visitee);
		}
	}
	// read-only pass never changes anything
	return false;
}

void FindLengthChecks::print(raw_ostream &sink, const Module *module) const {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	for (const Function &func : *module) {
		const ArgumentToMaxIndexMap map = maxIndexes.at(&func);
		sink << "Analyzing " << func.getName() << "\n";
		for (const Argument &arg : make_iterator_range(func.arg_begin(), func.arg_end())) {
			if (map.count(&arg))
				sink << "Argument " << arg.getName() << " has max index " << map.at(&arg) << '\n';
			else if (iiglue.isArray(arg))
				sink << "Argument " << arg.getName() << " has unknown max index.\n";
		}
	}
	
}