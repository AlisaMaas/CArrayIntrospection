#include "LengthInfo.hh"

#include <cassert>
#include <llvm/IR/Argument.h>
#include <llvm/IR/InstrTypes.h>
#include <sstream>


std::string LengthInfo::getTypeString(LengthType type) {
	switch(type) {
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
		assert(symbolicLength != nullptr);
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


std::string LengthInfo::toString() const {
	std::stringstream stream;
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
