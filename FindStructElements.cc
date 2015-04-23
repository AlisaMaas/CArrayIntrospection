#define DEBUG_TYPE "find-struct-elements"
#include "FindStructElements.hh"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <fstream>

using namespace llvm;
using namespace std;

struct FindStructsGEPVisitor : public InstVisitor<FindStructsGEPVisitor> {
	StructElementToValueSet &structCollection;
	std::vector<llvm::StructType*> &orderedTypes;
	FindStructsGEPVisitor(StructElementToValueSet &s,vector<StructType*> &o) : structCollection(s), orderedTypes(o) {
	}

	void visitGetElementPtrInst(GetElementPtrInst& gepi) {
		DEBUG(dbgs() << "Top of visitor\n");
		DEBUG(dbgs() << gepi << "\n");
		Type *pointer = gepi.getPointerOperandType()->getPointerElementType();
		if (gepi.getNumIndices() != 2 || !gepi.hasAllConstantIndices()) {
			DEBUG(dbgs() << "Aborting\n");
			return;
		}
		DEBUG(dbgs() << "is struct? " << pointer->isStructTy() << "\n");
		if (StructType *structTy = dyn_cast<StructType>(pointer)) {
			auto location = gepi.idx_begin();
			//assert that location points to zero here
			location++;
			ConstantInt *constant = dyn_cast<ConstantInt>(location->get());
			assert(constant != nullptr);
			int index = constant->getSExtValue();
			errs() << "Offset of " << index << "\n";
			pair<StructType*, int> p(structTy, index);
			structCollection[p].insert(&gepi);
			if (find(orderedTypes.begin(), orderedTypes.end(), structTy) == orderedTypes.end()) {
				orderedTypes.push_back(structTy);
			}
		}
		
	}
};

static const RegisterPass<FindStructElements> registration("find-struct-elements",
		"Organize GEP instructions referring to struct element accesses by struct element.",
		true, true);

char FindStructElements::ID;


inline FindStructElements::FindStructElements()
	: ModulePass(ID) {
}

void FindStructElements::getAnalysisUsage(AnalysisUsage &usage) const {
	// read-only pass never changes anything
	usage.setPreservesAll();
}


bool FindStructElements::runOnModule(Module &module) {
	DEBUG(dbgs() << "Top of runOnModule()\n");
	for (Function &func : module) {
		DEBUG(dbgs() << "Analyzing " << func.getName() << "\n");
		FindStructsGEPVisitor visitor(structElementCollections, orderedTypes);
		for(BasicBlock &visitee :  func) {
			DEBUG(dbgs() << "Visiting a new basic block...\n");
			visitor.visit(visitee);
		}
	}
	// read-only pass never changes anything
	return false;
}

void FindStructElements::print(raw_ostream &sink, const Module*) const {
	for (StructType *structTy : orderedTypes) {
		sink << structTy->getName() << ":\n";
		for (unsigned i = 0; i < structTy->getNumElements(); i++) {
			pair<StructType*, int> p(structTy, i);
			if (structElementCollections.count(p)) {
				for (const Value *inst : structElementCollections.at(p)) {
						sink << "\telement " << i << " accessed at " << inst->getName() << "\n";
				}
			}
		}
		
	}
	/*for (const Function &func : *module) {
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
	}*/
	
}
