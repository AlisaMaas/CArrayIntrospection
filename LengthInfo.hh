#ifndef INCLUDE_LENGTH_INFO_HH
#define INCLUDE_LENGTH_INFO_HH

#include "ValueSet.hh"

#include <memory>
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
	static const LengthInfo notFixedLength;
	static const LengthInfo inconsistent;
	static const LengthInfo sentinelTerminated;
	static LengthInfo parameterLength(long);
	static LengthInfo parameterLength(const std::shared_ptr<const ValueSet> &);
	static LengthInfo fixedLength(long);

	LengthInfo();

	LengthType type;
	long int length{-1};
	std::shared_ptr<const ValueSet> symbolicLength;
	static std::string getTypeString(const LengthType &);
	int getSymbolicLength() const;
	std::string toString() const;

private:
	LengthInfo(LengthType);
	LengthInfo(LengthType, long);
	LengthInfo(LengthType, const std::shared_ptr<const ValueSet> &);
};


////////////////////////////////////////////////////////////////////////


inline LengthInfo::LengthInfo()
	: LengthInfo{NO_LENGTH_VALUE} {
}


inline LengthInfo::LengthInfo(LengthType type)
	: type{type} {
}


inline LengthInfo::LengthInfo(LengthType type, long int length)
	: type{type},
	  length{length} {
}


inline LengthInfo::LengthInfo(LengthType type, const std::shared_ptr<const ValueSet> &symbolic)
	: type{type},
	  symbolicLength{symbolic} {
}


#endif // !INCLUDE_LENGTH_INFO_HH
