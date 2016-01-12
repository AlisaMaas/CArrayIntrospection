#include "LengthInfo.hh"

#include <cassert>
#include <llvm/IR/Argument.h>
#include <llvm/IR/InstrTypes.h>
#include <sstream>

using namespace std;


const LengthInfo LengthInfo::notFixedLength{NOT_FIXED_LENGTH};

const LengthInfo LengthInfo::inconsistent{INCONSISTENT};

const LengthInfo LengthInfo::sentinelTerminated{SENTINEL_TERMINATED, 0L};


LengthInfo LengthInfo::parameterLength(long slot) {
	assert(slot >= 0);
	return {PARAMETER_LENGTH, slot};
}


LengthInfo LengthInfo::parameterLength(const shared_ptr<const ValueSet> &symbolic) {
	return {PARAMETER_LENGTH, symbolic};
}


LengthInfo LengthInfo::fixedLength(long length) {
	assert(length >= 0);
	if (length == 0) return notFixedLength;
	return {FIXED_LENGTH, length};
}


string LengthInfo::getTypeString(const LengthType &type) {
	switch (type) {
	case NO_LENGTH_VALUE: return "None ";
	case PARAMETER_LENGTH: return "Param";
	case FIXED_LENGTH: return "Fixed";
	case NOT_FIXED_LENGTH: return "Not Fixed";
	case INCONSISTENT: return "Bad  ";
	case SENTINEL_TERMINATED: return "Sentinel";
	}
	return "Impossible";
}


int LengthInfo::getSymbolicLength() const {
	if (length == -1) {
		assert(symbolicLength);
		if (symbolicLength->size() == 1) {
			const llvm::Value *value = *symbolicLength->begin();
			while (true) {
				const llvm::CastInst *cast = llvm::dyn_cast<llvm::CastInst>(value);
				if (cast == nullptr) break;
				value = cast->getOperand(0);
			}
			const llvm::Argument *arg = llvm::dyn_cast<llvm::Argument>(value);
			if (arg != nullptr) {
				return arg->getArgNo();
			}
		}
		return -1;
	}
	else {
		return length;
	}
}


string LengthInfo::toString() const {
	stringstream stream;
	switch(type) {
	case NO_LENGTH_VALUE: return "No length value";
	case PARAMETER_LENGTH: 
		stream << "Parameter length of " << (length == -1? getSymbolicLength() : length);
		return stream.str();
	case FIXED_LENGTH: 
		stream << "Fixed length of " << length;
		return stream.str();
	case NOT_FIXED_LENGTH: return "Not fixed length";
	case INCONSISTENT: return "Inconsistent length";
	case SENTINEL_TERMINATED: 
		stream << "Sentinel-terminated by " << length;
		return stream.str();
	}
	return "Impossible";
}
