#ifndef INCLUDE_BACKTRACK_PHI_NODES_HH
#define INCLUDE_BACKTRACK_PHI_NODES_HH

#include <unordered_set>

namespace llvm {
	class Argument;
	class PHINode;
	class Value;
}


////////////////////////////////////////////////////////////////////////
//
//  generic base class for backtracking from values to the arguments
//  that may flow into them across zero or more phi nodes
//

class BacktrackPhiNodes {
public:
	void backtrack(const llvm::Value &, bool skipLoads=false);

protected:
	virtual void visit(const llvm::Value &) = 0;
	virtual bool shouldVisit(const llvm::Value &);
	virtual ~BacktrackPhiNodes();

private:
	std::unordered_set<const llvm::Value *> alreadySeen;
};


#endif	// !INCLUDE_BACKTRACK_PHI_NODES_HH
