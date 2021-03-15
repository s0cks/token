#ifndef TOKEN_MOCK_BLOCKCHAIN_H
#define TOKEN_MOCK_BLOCKCHAIN_H

#include <gmock/gmock.h>
#include "blockchain.h"

namespace token{
  class MockBlockChain : public BlockChain{
   public:
    MockBlockChain():
      BlockChain(){}
    ~MockBlockChain() = default;

    MOCK_METHOD(bool, HasBlock, (const Hash&), (const));
    MOCK_METHOD(BlockPtr, GetHead, (), (const));
    MOCK_METHOD(BlockPtr, GetBlock, (const Hash&), (const));
  };
}

#endif//TOKEN_MOCK_BLOCKCHAIN_H