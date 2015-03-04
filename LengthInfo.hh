#ifndef INCLUDE_LENGTH_INFO_HH
#define INCLUDE_LENGTH_INFO_HH
#include <string>

enum LengthType {
	NO_LENGTH_VALUE,
	PARAMETER_LENGTH,
	FIXED_LENGTH,
	INCONSISTENT
};

class LengthInfo {
	public: 
		LengthInfo(LengthType t, long int l) : type(t), length(l) {}
		LengthInfo() {
			type = NO_LENGTH_VALUE;
			length = -1;
		}
		LengthType type;
		long int length;
		static std::string getTypeString(LengthType type) {
			switch(type) {
				case NO_LENGTH_VALUE: return "None ";
				case PARAMETER_LENGTH: return "Param";
				case FIXED_LENGTH: return "Fixed";
				case INCONSISTENT: return "Bad  ";
			}
			return "Impossible";
		}
		std::string toString() {
			switch(type) {
				case NO_LENGTH_VALUE: return "No length value";
				case PARAMETER_LENGTH: return "Parameter length of " + length;
				case FIXED_LENGTH: return "Fixed length of " + length;
				case INCONSISTENT: return "Inconsistent length";
			}
			return "Impossible";
		}
};

#endif // !INCLUDE_LENGTH_INFO_HH