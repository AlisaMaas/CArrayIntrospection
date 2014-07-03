#include "../IIGlueReader.hh"
#include "../mocks/llvm/IR/Module.hh"

#include <gtest/gtest.h>


namespace
{
  // reusable test fixture for IIGlueReader tests
  class IIGlueReaderTest : public ::testing::Test
  {
  private:
    llvm::StringRef id;
    llvm::LLVMContext context;

  public:
    IIGlueReaderTest();
    mock::llvm::Module module;
    IIGlueReader reader;
  };
}


IIGlueReaderTest::IIGlueReaderTest()
  : module(id, context)
{
}


TEST_F(IIGlueReaderTest, InitializationChangesNothing)
{
  // IIGlueReader does pure analysis, never modifying bitcode
  ASSERT_FALSE(reader.doInitialization(module));
}


TEST_F(IIGlueReaderTest, NoFileToRead)
{
  // With no file named to read, IIGlueReader should print a warning
  // (not verified here) and return, doing no additional work.
  ASSERT_FALSE(reader.runOnModule(module));
}


TEST_F(IIGlueReaderTest, MissingFunction)
{
  // If the iiglue analysis results mention a function that we cannot
  // find in the bitcode, IIGlueReader should print a warning (not
  // verified here) and continue on with any other functions.

  // !!!: the call expectation below does not actually work correctly,
  // because llvm::Module::getFunction is non-virtual.
  const llvm::StringRef funcName = "noSuchFunction";
  EXPECT_CALL(module, getFunction(funcName));

  IIGlueReader::iiglueFileName = "tests/iiglue-unit-MissingFunction.json";
  ASSERT_FALSE(reader.runOnModule(module));
}
