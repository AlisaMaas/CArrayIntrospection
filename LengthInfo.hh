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
	INCONSISTENT,
	SENTINEL_TERMINATED
};

class LengthInfo {
	public: 
		LengthInfo(LengthType t, long int l) : type(t), length(l), symbolicLength(nullptr) {}
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
				case INCONSISTENT: return "Bad  ";
				case SENTINEL_TERMINATED: return "Sentinel";
			}
			return "Impossible";
		}
		void transformSymbolicLength() {
            if (length == -1) {
                assert(symbolicLength != nullptr);
                if (symbolicLength->size() == 1) {
                    const llvm::Argument *arg = llvm::dyn_cast<llvm::Argument>(*symbolicLength->begin());
                    if (arg != nullptr) {
                        length = arg->getArgNo();
                    }
                }
            }
		}
		std::string toString() {
			std::stringstream stream;
			switch(type) {
				case NO_LENGTH_VALUE: return "No length value";
				case PARAMETER_LENGTH: 
				transformSymbolicLength();
				stream << "Parameter length of " << length;
				return stream.str();
				case FIXED_LENGTH: 
				stream << "Fixed length of " << length;
				return stream.str();
				case INCONSISTENT: return "Inconsistent length";
				case SENTINEL_TERMINATED: 
				stream << "Sentinel-terminated by " << length;
				return stream.str();
			}
			return "Impossible";
		}
};

#endif // !INCLUDE_LENGTH_INFO_HH