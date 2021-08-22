#include <gtest/gtest.h>
#include "block.h"

namespace token{
  TEST(BlockTest, TestSerialization){
    auto timestamp = Clock::now();
    auto height = 0;
    auto previous_hash = Hash::GenerateNonce();
    IndexedTransactionSet transactions = {};
    auto a = Block::NewInstance(height, previous_hash, timestamp, transactions);
    LOG(INFO) << "hash: " << a->hash();

    auto data = a->ToBuffer();
    auto b = Block::From(data);

    ASSERT_EQ(a->hash(), b->hash());
    //TODO:
    // - check timestamp
    // - check height
    // - check previous_hash
    // - check transactions
  }
}