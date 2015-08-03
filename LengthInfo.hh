#ifndef INCLUDE_LENGTH_INFO_HH
#define INCLUDE_LENGTH_INFO_HH
#include <string>
#include <sstream>

enum LengthType {
	NO_LENGTH_VALUE,
	PARAMETER_LENGTH,
	FIXED_LENGTH,
	INCONSISTENT,
	SENTINEL_TERMINATED
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
				case SENTINEL_TERMINATED: return "Sentinel";
			}
			return "Impossible";
		}
		std::string toString() {
			std::stringstream stream;
			switch(type) {
				case NO_LENGTH_VALUE: return "No length value";
				case PARAMETER_LENGTH: 
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