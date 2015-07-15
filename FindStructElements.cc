#define DEBUG_TYPE "find-struct-elements"
#include "FindStructElements.hh"
#include "NameCompare.hh"

#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>
#include <fstream>
#include <sstream>

using namespace llvm;
using namespace std;


string str(const StructElement *element) {
    std::stringstream stream;
    stream << "struct " << element->first->getName().str() << "[" << element->second << "]";
    return stream.str();
}

StructElement* getStructElement(const llvm::Value *value) {
    //const LoadInst *load;
    /*while ((load = dyn_cast<LoadInst>(value))) {
        value = load->getPointerOperand();
    }
    const StoreInst *store;
     while ((store = dyn_cast<StoreInst>(value))) {
        value = store->getPointerOperand();
    }*/
    const llvm::GetElementPtrInst *gepi;
    if ((gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(value))) {
        llvm::Type *pointer = gepi->getPointerOperandType()->getPointerElementType();
        if (gepi->getNumIndices() != 2 || !gepi->hasAllConstantIndices()) {
            DEBUG(llvm::dbgs() << "Aborting\n");
            return nullptr;
        }
        DEBUG(llvm::dbgs() << "is struct? " << pointer->isStructTy() << "\n");
        if (llvm::StructType *structTy = llvm::dyn_cast<llvm::StructType>(pointer)) {
            auto location = gepi->idx_begin();
            //assert that location points to zero here
            location++;
            llvm::ConstantInt *constant = llvm::dyn_cast<llvm::ConstantInt>(location->get());
            assert(constant != nullptr);
            int index = constant->getSExtValue();
            DEBUG(llvm::dbgs() << "Offset of " << index << "\n");
            StructElement *p = new std::pair<llvm::StructType*, int>(structTy, index);
            return p;
        }
    }
    return nullptr;
}

struct FindStructsGEPVisitor : public InstVisitor<FindStructsGEPVisitor> {
    StructElementToValueSet &structCollection;
    std::vector<llvm::StructType*> &orderedTypes;
    FindStructsGEPVisitor(StructElementToValueSet &s,vector<StructType*> &o) : structCollection(s), orderedTypes(o) {}

    void visitGetElementPtrInst(GetElementPtrInst& gepi) {
        DEBUG(dbgs() << "Top of visitor\n");
        DEBUG(dbgs() << gepi << "\n");

        StructElement* element = getStructElement(&gepi);
        if (!element) return;
        if (!structCollection.count(*element)) {
            structCollection[*element] = new set<const Value*>();
        }
        structCollection[*element]->insert(&gepi);
        if (find(orderedTypes.begin(), orderedTypes.end(), element->first) == orderedTypes.end()) {
            orderedTypes.push_back(element->first);
        }
        
    }
};

static const RegisterPass<FindStructElements> registration("find-struct-elements",
        "Organize GEP instructions referring to struct element accesses by struct element.",
        true, true);

char FindStructElements::ID;

 FindStructElements::FindStructElements()
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

void FindStructElements::print(raw_ostream &sink, const Module* ) const {
    for (StructType *structTy : orderedTypes) {
        sink << structTy->getName() << ":\n";
        for (unsigned i = 0; i < structTy->getNumElements(); i++) {
            pair<StructType*, int> p(structTy, i);
	    const auto found {structElementCollections.find(p)};
	    if (found != structElementCollections.end()) {
		    const ValueSet &values {*found->second};
		    const std::multiset<const llvm::Value *, NameCompare> sortedValues {begin(values), end(values)};
		    for (const auto inst : sortedValues)
			    sink << "\telement " << i << " accessed at " << inst->getName() << "\n";
	    }
        }
        
    }
}
