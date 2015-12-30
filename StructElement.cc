#define DEBUG_TYPE "struct-element"

#include "StructElement.hh"

#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_ostream.h>

using namespace llvm;


StructElement* StructElement::get(const llvm::Value &value)
{
	//const LoadInst *load;
	/*while ((load = dyn_cast<LoadInst>(&value))) {
	  value = load->getPointerOperand();
	  }
	  const StoreInst *store;
	  while ((store = dyn_cast<StoreInst>(&value))) {
	  value = store->getPointerOperand();
	  }*/
	if (const llvm::GetElementPtrInst * const gepi = llvm::dyn_cast<llvm::GetElementPtrInst>(&value)) {
		const llvm::Type * const pointer = gepi->getPointerOperandType()->getPointerElementType();
		if (gepi->getNumIndices() != 2 || !gepi->hasAllConstantIndices()) {
			DEBUG(llvm::dbgs() << "Aborting\n");
			return nullptr;
		}
		DEBUG(llvm::dbgs() << "is struct? " << pointer->isStructTy() << "\n");
		if (const llvm::StructType * const structTy = llvm::dyn_cast<llvm::StructType>(pointer)) {
			auto location = gepi->idx_begin();
			//assert that location points to zero here
			location++;
			const llvm::ConstantInt * const constant = llvm::dyn_cast<llvm::ConstantInt>(location->get());
			assert(constant != nullptr);
			int index = constant->getSExtValue();
			DEBUG(llvm::dbgs() << "Offset of " << index << "\n");
			return new StructElement(*structTy, index);
		}
	}
	return nullptr;
}


raw_ostream &operator<<(raw_ostream &sink, const StructElement &element)
{
	return sink << "struct " << element.structure.getName()
		    << '[' << element.index << ']';
}
