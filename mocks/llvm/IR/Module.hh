#ifndef INCLUDE_MOCKS_IR_MODULE_HH
#define INCLUDE_MOCKS_IR_MODULE_HH


#include <gmock/gmock.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

namespace mock
{
  namespace llvm
  {
    class Module : public ::llvm::Module
    {
    public:
      using ::llvm::Module::Module;

      // !!!: this mocked method does not actually work correctly,
      // because llvm::Module::getFunction is non-virtual.
      MOCK_CONST_METHOD1(getFunction, ::llvm::Function * (::llvm::StringRef));
    };
  }
}


#endif	// !INCLUDE_MOCKS_IR_MODULE_HH
