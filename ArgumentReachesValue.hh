#ifndef INCLUDE_ARGUMENT_REACHES_VALUE_HH
#define INCLUDE_ARGUMENT_REACHES_VALUE_HH

#include "BacktrackPhiNodes.hh"


////////////////////////////////////////////////////////////////////////
//
//  test whether a specific argument may flow into a specific value
//  across zero or more phi nodes
//


bool argumentReachesValue(const llvm::Argument &goal, const llvm::Value &start);


#endif	// !INCLUDE_ARGUMENT_REACHES_VALUE_HH
