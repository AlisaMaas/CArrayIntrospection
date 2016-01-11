#define DEBUG_TYPE "check-get-element-ptr-visitor"

#include "CheckGetElementPtrVisitor.hh"
#include "SRA/SymbolicRangeAnalysis.h"
#include "ValueSetsReachingValue.hh"

#include <boost/lambda/core.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/iterator_range.hpp>
#include <llvm/Support/Debug.h>
#include <llvm/Support/raw_os_ostream.h>
#include <memory>
#include <unordered_map>

#if (1000 * LLVM_VERSION_MAJOR + LLVM_VERSION_MINOR) >= 3005
#include <llvm/IR/InstIterator.h>
#else  // LLVM 3.4 or earlier
#include <llvm/Support/InstIterator.h>
#endif	// LLVM 3.4 or earlier


using namespace boost::adaptors;
using namespace llvm;
using namespace std;


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


////////////////////////////////////////////////////////////////////////


template <typename VS>
const ValueSet *CheckGetElementPtrVisitor<VS>::getValueLength(llvm::Value *first, llvm::Value *second, const llvm::Value *basePointer) {
	const ValueSetSet<const ValueSet *> reaching = valueSetsReachingValue(*first, valueSets);
	if (reaching.empty()) return nullptr;
	else if (reaching.size() == 1) {
		ConstantInt* c;
		if ((c = dyn_cast<ConstantInt>(second)) && c->isMinusOne()) {
			return *reaching.begin();
		}
		else return nullptr;
	}
	else {
		const ValueSet * const valueSet { valueSets.getValueSetFromValue(basePointer) };
		notConstantBounded.insert(valueSet);
		notParameterBounded.insert(valueSet);
		return nullptr;
	}
}


template <typename VS>
bool CheckGetElementPtrVisitor<VS>::matchAddPattern(llvm::Value *value, llvm::Value *basePointer) {
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
				const ValueSet *valueSet = valueSets.getValueSetFromValue(basePointer);
				const auto emplaced = lengths.emplace(valueSet, length);
				if (emplaced.second)
					// no prior value
					return true;
				else if (emplaced.first->second == length)
					// prior value already same
					return true;
				else {
					notParameterBounded.insert(valueSet);
				}
			}

		}
	}
	return false;
}


template <typename VS>
CheckGetElementPtrVisitor<VS>::CheckGetElementPtrVisitor(
	ValueSetToMaxIndexMap &map, const SymbolicRangeAnalysis &ra,
	const llvm::Module &m, LengthValueSetMap &l, const ValueSetSet<VS> &v)
	: maxIndexes(map),
	  lengths(l),
	  rangeAnalysis(ra),
	  valueSets(v),
	  module(m),
	  functionsToCallsites(collectFunctionCalls(m))
{
}


template <typename VS>
CheckGetElementPtrVisitor<VS>::~CheckGetElementPtrVisitor() {
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
}


template <typename VS>
void CheckGetElementPtrVisitor<VS>::visitGetElementPtrInst(llvm::GetElementPtrInst& gepi) {
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
	placeHolder.reset(BasicBlock::Create(module.getContext()));
	DEBUG(dbgs() << "Top of visitor\n");
	Value *pointer = gepi.getPointerOperand();
	DEBUG(dbgs() << "Pointer operand obtained: " << *pointer << "\n");

	const ValueSet *valueSet = valueSets.getValueSetFromValue(pointer);
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
	Value *index = gepi.idx_begin()->get();
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
		long &max{maxIndexes[valueSet]};
		if (index > max) {
			DEBUG(dbgs() << "Yay range analysis! Adding to the map!\n");
			max = index;
		}
	}
	else {
		DEBUG(dbgs() << "Not constant index\n");
		notConstantBounded.insert(valueSet);
		DEBUG(dbgs() << "Index in question = " << *index << "\n");
		DEBUG(dbgs() << "Is integer type? " << index->getType()->isIntegerTy() << "\n");
		Value *symbolicIndexInst = rangeAnalysis.getRangeValuesFor(index, IRBuilder<>(placeHolder.get())).second;
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



////////////////////////////////////////////////////////////////////////


template class CheckGetElementPtrVisitor<const ValueSet *>;
template class CheckGetElementPtrVisitor<      ValueSet  >;


////////////////////////////////////////////////////////////////////////


const shared_ptr<const ValueSet> SharedCheckGetElementPtrVisitor::getValueLength(llvm::Value *first, llvm::Value *second, const llvm::Value *basePointer) {
	const ValueSetSet<shared_ptr<const ValueSet>> reaching = valueSetsReachingValue(*first, valueSets);
	if (reaching.empty()) return nullptr;
	else if (reaching.size() == 1) {
		ConstantInt* c;
		if ((c = dyn_cast<ConstantInt>(second)) && c->isMinusOne()) {
			return *reaching.begin();
		}
		else return {};
	}
	else {
		const shared_ptr<const ValueSet> valueSet{valueSets.getValueSetFromValue(basePointer)};
		notConstantBounded.insert(valueSet);
		notParameterBounded.insert(valueSet);
		return {};
	}
}


bool SharedCheckGetElementPtrVisitor::matchAddPattern(llvm::Value *value, llvm::Value *basePointer) {
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
			shared_ptr<const ValueSet> length{getValueLength(firstOperand, secondOperand, basePointer)};
			if (!length) length = getValueLength(secondOperand, firstOperand, basePointer);
			if (length) {
				DEBUG(dbgs() << "Hey, look, an argument length! \n");
				const shared_ptr<const ValueSet> valueSet{valueSets.getValueSetFromValue(basePointer)};
				const auto emplaced = lengths.emplace(valueSet.get(), length.get());
				if (emplaced.second)
					// no prior value
					return true;
				else if (emplaced.first->second == length.get())
					// prior value already same
					return true;
				else {
					notParameterBounded.insert(valueSet);
				}
			}

		}
	}
	return false;
}


SharedCheckGetElementPtrVisitor::SharedCheckGetElementPtrVisitor(
	ValueSetToMaxIndexMap &map, const SymbolicRangeAnalysis &ra,
	const llvm::Module &m, LengthValueSetMap &l, const ValueSetSet<shared_ptr<const ValueSet>> &v)
	: maxIndexes(map),
	  lengths(l),
	  rangeAnalysis(ra),
	  valueSets(v),
	  module(m),
	  functionsToCallsites(collectFunctionCalls(m))
{
}


SharedCheckGetElementPtrVisitor::~SharedCheckGetElementPtrVisitor() {
	DEBUG(dbgs() << "destructor\n");
	for (const auto &v : notConstantBounded) {
		DEBUG(dbgs() << "Kicking out some constants\n");
		maxIndexes.erase(v.get());
	}
	DEBUG(dbgs() << "Done with constants\n");
	for (const auto &v : notParameterBounded) {
		lengths.erase(v.get());
	}
	DEBUG(dbgs() << "Finished with the delete\n");
}


void SharedCheckGetElementPtrVisitor::visitGetElementPtrInst(llvm::GetElementPtrInst& gepi) {
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
	placeHolder.reset(BasicBlock::Create(module.getContext()));
	DEBUG(dbgs() << "Top of visitor\n");
	Value *pointer = gepi.getPointerOperand();
	DEBUG(dbgs() << "Pointer operand obtained: " << *pointer << "\n");

	const auto valueSet{valueSets.getValueSetFromValue(pointer)};
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
	Value *index = gepi.idx_begin()->get();
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
		long &max{maxIndexes[valueSet.get()]};
		if (index > max) {
			DEBUG(dbgs() << "Yay range analysis! Adding to the map!\n");
			max = index;
		}
	}
	else {
		DEBUG(dbgs() << "Not constant index\n");
		notConstantBounded.insert(valueSet);
		DEBUG(dbgs() << "Index in question = " << *index << "\n");
		DEBUG(dbgs() << "Is integer type? " << index->getType()->isIntegerTy() << "\n");
		Value *symbolicIndexInst = rangeAnalysis.getRangeValuesFor(index, IRBuilder<>(placeHolder.get())).second;
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
