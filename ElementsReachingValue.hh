#ifndef INCLUDE_ELEMENTS_REACHING_VALUE_HH
#define INCLUDE_ELEMENTS_REACHING_VALUE_HH

#include "BacktrackPhiNodes.hh"
#include "FindStructElements.hh"

#include <llvm/Support/Casting.h>
#include <llvm/Support/Debug.h>
#include <unordered_set>

////////////////////////////////////////////////////////////////////////
//
//  collect the set of all struct elements that may flow to a given value
//  across zero or more phi nodes
//

typedef std::unordered_set<const StructElement*> ElementSet;

class ElementsReachingValue : public BacktrackPhiNodes {
public:
        void visit(const llvm::Value &) final override;
        bool shouldVisit(const llvm::Value &value) final override;
        ElementSet result;
        static ElementSet elementsReachingValue(const llvm::Value &start);
};

inline void ElementsReachingValue::visit(const llvm::Value &reached) {
        DEBUG(llvm::dbgs() << "Found a thing!\n");
        result.insert(getStructElement(&reached));
}
    
inline bool ElementsReachingValue::shouldVisit(const llvm::Value &value) {
        return getStructElement(&value) != nullptr;
}

inline static ElementSet elementsReachingValue(const llvm::Value &start) {
        ElementsReachingValue explorer;
        explorer.backtrack(start);
        return std::move(explorer.result);
}

#endif    // !INCLUDE_ELEMENTS_REACHING_VALUE_HH
