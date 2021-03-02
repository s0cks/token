#ifndef TOKEN_TEST_BLOCKCHAIN_H
#define TOKEN_TEST_BLOCKCHAIN_H

#include "test_suite.h"
#include "blockchain.h"

namespace token{
  class BlockChainTest : public ::testing::Test{
   protected:
    BlockChainTest() = default;
   public:
    ~BlockChainTest() = default;
  };
}

#endif//TOKEN_TEST_BLOCKCHAIN_H