#ifndef INCLUDE_IIGLUE_READER_HH
#define INCLUDE_IIGLUE_READER_HH

#include <llvm/Pass.h>
#include <llvm/Support/CommandLine.h>
#include <unordered_set>

namespace llvm
{
  class Argument;
}


////////////////////////////////////////////////////////////////////////
//
//  read iiglue analysis output and tie it to LLVM data structures
//

class IIGlueReader : public llvm::ModulePass
{
public:
  // standard LLVM pass interface
  IIGlueReader();
  static char ID;
  void getAnalysisUsage(llvm::AnalysisUsage &) const override final;
  bool runOnModule(llvm::Module &) override final;
  void print(llvm::raw_ostream &, const llvm::Module *) const;

  // command-line flag for "opt" and other tools
  static llvm::cl::opt<std::string> iiglueFileName;

  // convenience methods to query loaded iiglue annotations
  bool isArray(const llvm::Argument &) const;

private:
  // formal function arguments marked as arrays by iiglue
  std::unordered_set<const llvm::Argument *> arrayArguments;
};


////////////////////////////////////////////////////////////////////////


inline IIGlueReader::IIGlueReader()
  : ModulePass(ID)
{
}


inline bool IIGlueReader::isArray(const llvm::Argument &argument) const
{
  return arrayArguments.find(&argument) != arrayArguments.end();
}


#endif // !INCLUDE_IIGLUE_READER_HH
