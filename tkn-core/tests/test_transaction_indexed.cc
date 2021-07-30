#include <gtest/gtest.h>
#include "transaction_indexed.h"

namespace token{
  TEST(IndexedTransactionTest, TestSerialization){
    auto timestamp = Clock::now();
    auto index = 0;
    std::vector<Input> inputs = {};
    std::vector<Output> outputs = {
        Output("TestUser", "TestToken1"),
        Output("TestUser", "TestToken2"),
        Output("TestUser", "TestToken3"),
    };

    IndexedTransaction a(timestamp, inputs, outputs, index);
    LOG(INFO) << "timestamp: " << ToUnixTimestamp(timestamp);
    LOG(INFO) << "index: " << index;
    LOG(INFO) << "inputs: " << inputs.size();
    LOG(INFO) << "outputs: " << outputs.size();

    auto data = a.ToBuffer();
    IndexedTransaction b(data);

    ASSERT_EQ(a, b);
    //TODO:
    // - check timestamp
    // - check inputs
    // - check outputs
  }
}