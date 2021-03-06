#ifndef INCLUDE_STRUCT_ELEMENT_HH
#define INCLUDE_STRUCT_ELEMENT_HH

#include <boost/optional.hpp>
#include <llvm/IR/DerivedTypes.h>


class StructElement {
public:
	StructElement(const llvm::StructType &, unsigned);
	static boost::optional<StructElement> get(const llvm::Value &);

	const llvm::StructType &structure;
	unsigned index;
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
