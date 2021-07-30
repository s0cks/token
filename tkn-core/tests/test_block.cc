#include <gtest/gtest.h>
#include "block.h"

namespace token{
  TEST(BlockTest, TestSerialization){
    auto timestamp = Clock::now();
    auto height = 0;
    auto previous_hash = Hash::GenerateNonce();
    IndexedTransactionSet transactions = {};
    Block a(timestamp, height, previous_hash, transactions);
    LOG(INFO) << "hash: " << a.hash();

    auto data = a.ToBuffer();
    Block b(data);

    ASSERT_EQ(a, b);
    //TODO:
    // - check timestamp
    // - check height
    // - check previous_hash
    // - check transactions
  }
}