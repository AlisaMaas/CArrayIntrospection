#include "BacktrackPhiNodes.hh"

#include <boost/range/iterator_range_core.hpp>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstrTypes.h>
#include "llvm/Support/Debug.h"

using namespace boost;
using namespace llvm;


BacktrackPhiNodes::~BacktrackPhiNodes() {
}


void BacktrackPhiNodes::backtrack(const Value &value, bool searchUseless) {
	if (!alreadySeen.insert(&value).second)
		return;

	if (shouldVisit(value))
		visit(value);
	if (const PHINode * phi = dyn_cast<PHINode>(&value)) {
		const auto operands = make_iterator_range(phi->op_begin(), phi->op_end());
		for (const Use &operand : operands)
			backtrack(*operand);
	}
	if (const CastInst *cast = dyn_cast<CastInst>(&value)) {
        const auto operands = make_iterator_range(cast->op_begin(), cast->op_end());
        for (const Use &operand : operands) {
            if (operand != nullptr)
                backtrack(*operand);
        }
	}
	
	
	if (searchUseless) {
        if (const GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(&value))
            if (gep->getType() == gep->getPointerOperandType()) {
                errs() << *gep << "\n";
                errs() << "Gep type is " << *gep->getType() << "\n";
                errs() << "operand " << *gep->getPointerOperand() << "\n";
                errs() << "operand type " << *gep->getPointerOperandType() << "\n";
                if (gep->getPointerOperand()!= nullptr) {
                    bool useless = true;
                    for (const User *user : gep->users()) {
                        if (StoreInst::classof(user)) {
                            useless = false;
                            break;
                        }
                        if (LoadInst::classof(user)) {
                            useless = false;
                            break;
                        }
                        if (GetElementPtrInst::classof(user)) {
                            useless = false;
                            break;
                        }
                    }
                    if (useless)
                        backtrack(*gep->getPointerOperand());
                }
            }
    }
    else {
        if (const GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(&value))
           if (gep->hasAllZeroIndices() && gep->getNumIndices() == 1)
               if (gep->getPointerOperand()!= nullptr)
                       backtrack(*gep->getPointerOperand());
    }
	if (const SExtInst *se = dyn_cast<SExtInst>(&value)) {
	    if (se->getOperand(0) != nullptr)
	        backtrack(*se->getOperand(0));
	}
	if (false) {
        if (const  LoadInst * const load = dyn_cast<LoadInst>(&value)) {
            backtrack(*load->getPointerOperand());
        }
	}
	/*else if (const StoreInst * const store = dyn_cast<StoreInst>(&value)) {
	    backtrack(*store->getPointerOperand());
	}*/
}

bool BacktrackPhiNodes::shouldVisit(const llvm::Value &value) {
	return (dyn_cast<Argument>(&value) != nullptr);
}
