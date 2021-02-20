#ifndef TOKEN_TEST_BLOCK_H
#define TOKEN_TEST_BLOCK_H

#include "block.h"
#include "test_suite.h"

namespace token{
  class BlockTest : public BinaryObjectTest<Block>{
   protected:
    static const std::string kExpectedHash;

    BlockTest() = default;

    BlockPtr GetObject() const{
      return Block::Genesis();
    }

    BlockPtr GetRandomObject() const{
      BlockPtr parent = Block::Genesis();
      IndexedTransactionSet transactions = {};
      Timestamp timestamp = Clock::now();
      return Block::FromParent(parent, transactions, timestamp);
    }
   public:
    ~BlockTest() = default;
  };
}

#endif//TOKEN_TEST_BLOCK_H