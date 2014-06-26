#include <llvm/Pass.h>

using namespace llvm;


namespace
{
  class IIGlueReader : public ModulePass
  {
  public:
    IIGlueReader();
    static char ID;
    void getAnalysisUsage(AnalysisUsage &) const override final;
    bool runOnModule(Module &) override final;
  };
}


char IIGlueReader::ID;

static const RegisterPass<IIGlueReader>
registration("iiglue-reader",
	     "Read iiglue analysis output and add it as metadata on corresponding LLVM entities",
	     true, true);


inline IIGlueReader::IIGlueReader()
  : ModulePass(ID)
{
}


void IIGlueReader::getAnalysisUsage(AnalysisUsage &usage) const
{
  // read-only pass never changes anything
  usage.setPreservesAll();
}


bool IIGlueReader::runOnModule(Module &)
{
  return false;
}
