#include "StructElement.hh"
#include <llvm/Support/raw_ostream.h>

using namespace llvm;


raw_ostream &operator<<(raw_ostream &sink, const StructElement &element)
{
	return sink << "struct " << element.structure.getName()
		    << '[' << element.index << ']';
}
