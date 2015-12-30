#define DEBUG_TYPE "find-struct-elements"
#include "FindStructElements.hh"
#include "NameCompare.hh"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include <llvm/IR/InstVisitor.h>
#pragma GCC diagnostic pop

#include <fstream>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_os_ostream.h>
#include <sstream>

using namespace llvm;
using namespace std;


struct FindStructsGEPVisitor : public InstVisitor<FindStructsGEPVisitor> {
    StructElementToValueSet &structCollection;
    std::vector<const StructType *> &orderedTypes;
    FindStructsGEPVisitor(StructElementToValueSet &s,vector<const StructType *> &o) : structCollection(s), orderedTypes(o) {}

    void visitGetElementPtrInst(GetElementPtrInst &gepi) {
        DEBUG(dbgs() << "Top of visitor\n");
        DEBUG(dbgs() << gepi << "\n");

        const auto element = StructElement::get(gepi);
        if (!element) return;
        if (!structCollection.count(*element)) {
            structCollection[*element] = new set<const Value*>();
        }
        structCollection[*element]->insert(&gepi);
        if (find(orderedTypes.begin(), orderedTypes.end(), &element->structure) == orderedTypes.end()) {
            orderedTypes.push_back(&element->structure);
        }
        
    }
};

static const RegisterPass<FindStructElements> registration("find-struct-elements",
        "Organize GEP instructions referring to struct element accesses by struct element.",
        true, true);

char FindStructElements::ID;
static llvm::cl::opt<std::string>
    testOutputName("test-find-struct-elements",
        llvm::cl::Optional,
        llvm::cl::value_desc("filename"),
        llvm::cl::desc("Filename to write results to for regression tests"));

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

void FindStructElements::print(raw_ostream &sink, const Module* ) const {
    for (const StructType *structTy : orderedTypes) {
        sink << structTy->getName() << ":\n";
        for (unsigned i = 0; i < structTy->getNumElements(); i++) {
	    const StructElement p { *structTy, i };
	    const auto found(structElementCollections.find(p));
	    if (found != structElementCollections.end())
	        sink << "\telement " << i << " accessed\n";
        }
        
    }
}
