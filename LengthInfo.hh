#ifndef INCLUDE_LENGTH_INFO_HH
#define INCLUDE_LENGTH_INFO_HH
#include <llvm/IR/Argument.h>
//#include <llvm/Support/Casting.h>
#include <string>
#include <sstream>

#include "ValueSet.hh"

enum LengthType {
	NO_LENGTH_VALUE,
	PARAMETER_LENGTH,
	FIXED_LENGTH,
	NOT_FIXED_LENGTH,
	INCONSISTENT,
	SENTINEL_TERMINATED
};

class LengthInfo {
	public: 
		LengthInfo(LengthType t, long int l) : type(t), length(l), symbolicLength(nullptr) {
		    if (type == FIXED_LENGTH && length == 0) {
		        type = NOT_FIXED_LENGTH;
		    }
		}
		LengthInfo(LengthType t, const ValueSet *symbolic, long int l) : type(t), length(l), symbolicLength(symbolic) {}
		LengthInfo() {
			type = NO_LENGTH_VALUE;
			length = -1;
		}
		LengthType type;
		long int length;
		const ValueSet *symbolicLength;
		static std::string getTypeString(LengthType type) {
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
		int getSymbolicLength() const {
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
		std::string toString() const {
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
};

#endif // !INCLUDE_LENGTH_INFO_HH