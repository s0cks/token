#ifndef TOKEN_TEST_BLOCK_FILE_H
#define TOKEN_TEST_BLOCK_FILE_H

#include "test_suite.h"
#include "block_file.h"

namespace token{
  class BlockFileTest : public ::testing::Test{
   protected:
    FILE* file_;

    BlockFileTest() = default;

    void SetUp(){
      file_ = tmpfile();
      ASSERT_NE(file_, nullptr);
    }

    void TearDown(){
      ASSERT_TRUE(Close(file_));
    }
   public:
    ~BlockFileTest() = default;
  };
}

#endif//TOKEN_TEST_BLOCK_FILE_H