#include "UpperBoundIndexing.hh"
#include "IIGlueReader.hh"
#include "PatternMatch-extras.hh"

#include <boost/container/flat_set.hpp>
#include <boost/range/adaptor/indirected.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/InstVisitor.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Debug.h>

using namespace boost::adaptors;
using namespace boost;
using namespace llvm;
using namespace llvm::PatternMatch;
using namespace std;

struct FindUpperBoundVisitor : public InstVisitor<FindUpperBoundVisitor> {
	const Value *indVar;
	const Loop *loop;
	int upperBound;
	FindUpperBoundVisitor(const Value *node, const Loop *l){
		indVar = node;
		loop = l;
		upperBound = -1;
	}
	void visitBranchInst(BranchInst &br) {
		if (!br.isConditional())
			return;
		CmpInst *cond = dyn_cast<CmpInst>(br.getCondition());
		if (cond) {
			errs() << "Got a cmpinst\n";
			errs() << "num operands: " << cond->getNumOperands() << "\n";
			assert(cond->getNumOperands() == 2);
			Value* firstOp = cond->getOperand(0);
			Value* secondOp = cond->getOperand(1);
			errs() << "First op: " << *firstOp << "\n";
			errs() << "Second op: " << *secondOp << "\n";
			CmpInst::Predicate pred = cond->getPredicate();
			Value *temp;
			ConstantInt *val;
			int add = 0;
			BasicBlock *trueBlock = br.getSuccessor(0);
			BasicBlock *falseBlock = br.getSuccessor(1);
			if (!(loop->contains(trueBlock) && !loop->contains(falseBlock))) {
				errs() << "True and false blocks don't point to the right places.\n";
				errs() << "True: " << *trueBlock << "\n";
				errs() << "As compared to: " << loop->contains(trueBlock) << "\n";
				errs() << "False: " << *falseBlock << "\n";
				errs() << "As compared to: " << !loop->contains(falseBlock) << "\n";
			}
			errs() << "About to start switch.\n";
			switch(pred){
			case CmpInst::ICMP_NE:
				add = -1;
			case CmpInst::ICMP_EQ: //might need to be a little careful later about the replacement depending on whether this
			//is in fact in the guard.
				Value *bound;
				if (firstOp == indVar) {
					bound = secondOp;
				}
				else {
					bound = firstOp;
				}
				val = dyn_cast<ConstantInt>(bound);
				if (upperBound < val->getSExtValue() + add) {
					upperBound = val->getSExtValue() + add;
					}
				break;
			
			case CmpInst::ICMP_UGE:
			case CmpInst::ICMP_SGE:
				temp = firstOp;
				firstOp = secondOp;
				secondOp = temp;
			case CmpInst::ICMP_ULE:
			case CmpInst::ICMP_SLE:
				if (firstOp == indVar) {
					errs() << "It's an upper bound\n";
					val = dyn_cast<ConstantInt>(secondOp);
					if (upperBound < val->getSExtValue()) {
						upperBound = val->getSExtValue();
					}
				}
				
				else {
					errs() << "It's a lower bound, not an upper bound.\n";
				}
				
				break;
			case CmpInst::ICMP_SGT:
			case CmpInst::ICMP_UGT:
				temp = firstOp;
				firstOp = secondOp;
				secondOp = temp;
			case CmpInst::ICMP_SLT:
			case CmpInst::ICMP_ULT:
				if (secondOp == indVar) {
					errs() << "It's an upper bound\n";
					val = dyn_cast<ConstantInt>(firstOp);
					if (upperBound < val->getSExtValue()-1) {
						upperBound = val->getSExtValue()-1;
					}
				}
				
				else {
					errs() << "It's a lower bound, not an upper bound.\n";
				}
				break;
				
			default:
				errs() << "Unknown predicate: " << pred << "\n";
				break;
			
			}
			errs() << "Predicate: " << pred << "\n";
		}
		errs() << *br.getCondition() << "\n";		
	}
};

static const RegisterPass<UpperBoundIndexing> registration("boundify",
		"Replace indexes using the induction variable with the constant upper bound of the loop, if it exists.",
		true, true);

char UpperBoundIndexing::ID;


inline UpperBoundIndexing::UpperBoundIndexing()
	: ModulePass(ID) {
}

void UpperBoundIndexing::getAnalysisUsage(AnalysisUsage &usage) const {
	usage.addPreservedID(IIGlueReader::ID);
	usage.addRequired<LoopInfo>();
	usage.addRequired<IIGlueReader>();
}


bool UpperBoundIndexing::runOnModule(Module &module) {
	const IIGlueReader &iiglue = getAnalysis<IIGlueReader>();
	bool modified = false;
	errs() << "Top of runOnModule()\n";
	for (Function &func : module) {
		errs() << "Analyzing " << func.getName() << "\n";
		LoopInfo &LI = getAnalysis<LoopInfo>(func);
		// We must look through all the loops to determine if any of them contain a sentinel check.
		for (const Loop * const loop : LI) {
			errs() << "Got a loop\n";
			SmallVector<BasicBlock *, 4> exitingBlocks;
			loop->getExitingBlocks(exitingBlocks);
			PHINode *indVar = loop->getCanonicalInductionVariable();
			int upperBound = -1;
			if (indVar != nullptr) {
				FindUpperBoundVisitor visitor(indVar, loop);
				for (BasicBlock *exitingBlock : exitingBlocks) {
					TerminatorInst * const terminator = exitingBlock->getTerminator();
					errs() << *terminator << "\n";
					visitor.visit(terminator);
					errs() << "Upperbound: " << visitor.upperBound << "\n";
					//pattern match for something comparing the indvar to a constant.
					//if it matches, set upperBound to the constant
				}
				upperBound = visitor.upperBound;
			}
			if (upperBound < 0) {
				return false;
			}
			indVar->replaceAllUsesWith(ConstantInt::get(indVar->getType(), upperBound));
			modified = true;
		}
	}
	// read-only pass never changes anything
	return modified;
}


void UpperBoundIndexing::print(raw_ostream &sink, const Module *module) const {
	(void) sink;
	(void) module;
}