#ifndef INCLUDE_LENGTH_INFO_HH
#define INCLUDE_LENGTH_INFO_HH

#include "ValueSet.hh"

#include <string>


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
	LengthInfo(LengthType t, long int l)
		: type((t == FIXED_LENGTH && l == 0) ? NOT_FIXED_LENGTH : t),
		  length(l),
		  symbolicLength(nullptr) {
	}

	LengthInfo(LengthType t, const ValueSet *symbolic, long int l)
		: type(t),
		  length(l),
		  symbolicLength(symbolic) {
	}

	LengthInfo()
		: type(NO_LENGTH_VALUE),
		  length(-1) {
	}

	LengthType type;
	long int length;
	const ValueSet *symbolicLength;
	static std::string getTypeString(LengthType);
	int getSymbolicLength() const;
	std::string toString() const;
};

#endif // !INCLUDE_LENGTH_INFO_HH
