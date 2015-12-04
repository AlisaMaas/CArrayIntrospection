#include "NameCompare.hh"

#include <llvm/ADT/StringRef.h>

using namespace llvm;


bool NameCompare::operator()(const Value *x, const Value *y) const
{
	return x->getName() < y->getName();
}
