#ifndef TOKEN_MOCK_BLOCKCHAIN_H
#define TOKEN_MOCK_BLOCKCHAIN_H

#include <gmock/gmock.h>
#include "blockchain.h"

namespace token{
  class MockBlockChain;
  typedef std::shared_ptr<MockBlockChain> MockBlockChainPtr;

  class MockBlockChain : public BlockChain{
   public:
    MockBlockChain():
      BlockChain(){}
    ~MockBlockChain() = default;

    MOCK_METHOD(bool, HasBlock, (const Hash&), (const));
    MOCK_METHOD(BlockPtr, GetHead, (), (const));
    MOCK_METHOD(BlockPtr, GetBlock, (const Hash&), (const));

    static inline MockBlockChainPtr
    NewInstance(){
      return std::make_shared<MockBlockChain>();
    }
  };
}

#endif//TOKEN_MOCK_BLOCKCHAIN_H