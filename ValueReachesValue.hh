#ifndef INCLUDE_VALUE_REACHES_VALUE_HH
#define INCLUDE_VALUE_REACHES_VALUE_HH

#include "BacktrackPhiNodes.hh"

namespace llvm {
	class Value;
}


////////////////////////////////////////////////////////////////////////
//
//  test whether a specific argument may flow into a specific value
//  across zero or more phi nodes
//

bool valueReachesValue(const llvm::Value &goal, const llvm::Value &start, bool skipLoads=false);


#endif // !INCLUDE_VALUE_REACHES_VALUE_HH
