#ifndef TOKEN_TEST_BLOCK_HEADER_H
#define TOKEN_TEST_BLOCK_HEADER_H

#include "test_suite.h"
#include "block_header.h"

namespace token{
  class BlockHeaderTest : public ::testing::Test{
   protected:
    BlockHeaderTest():
      ::testing::Test(){}
   public:
    ~BlockHeaderTest() override = default;
  };
}

#endif//TOKEN_TEST_BLOCK_HEADER_H