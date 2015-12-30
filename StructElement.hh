#ifndef INCLUDE_STRUCT_ELEMENT_HH
#define INCLUDE_STRUCT_ELEMENT_HH

#include <llvm/IR/DerivedTypes.h>


class StructElement {
public:
	static StructElement *get(const llvm::Value &);
	const llvm::StructType &structure;
	unsigned index;

public:
	// TODO: make this private
	StructElement(const llvm::StructType &, unsigned);
};


bool operator<(const StructElement &, const StructElement &);

llvm::raw_ostream &operator<<(llvm::raw_ostream &, const StructElement &);


////////////////////////////////////////////////////////////////////////


inline StructElement::StructElement(const llvm::StructType &structure, unsigned index)
	: structure(structure),
	  index(index)
{
}


inline bool operator<(const StructElement &a, const StructElement &b)
{
	const auto aFieldwise = std::make_pair(&a.structure, a.index);
	const auto bFieldwise = std::make_pair(&b.structure, b.index);
	return aFieldwise < bFieldwise;
}


#endif // !INCLUDE_FIND_STRUCT_ELEMENTS_HH
