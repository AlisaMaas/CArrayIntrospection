#ifndef INCLUDE_NAME_COMPARE_HH
#define INCLUDE_NAME_COMPARE_HH

#include <llvm/IR/Value.h>


// compare two Value pointers using their names
class NameCompare
{
public:
	bool operator()(const llvm::Value *, const llvm::Value *) const;
};


#endif	// !INCLUDE_NAME_COMPARE_HH
