#include <gtest/gtest.h>
#include "transaction_unsigned.h"

namespace token{
  TEST(UnsignedTransactionTest, TestSerialization){
    auto timestamp = Clock::now();
    std::vector<Input> inputs = {};
    std::vector<Output> outputs = {
      Output("TestUser", "TestToken1"),
      Output("TestUser", "TestToken2"),
      Output("TestUser", "TestToken3"),
    };

    UnsignedTransaction a;//TODO: fix
    LOG(INFO) << "timestamp: " << ToUnixTimestamp(timestamp);
    LOG(INFO) << "inputs: " << inputs.size();
    LOG(INFO) << "outputs: " << outputs.size();

    auto data = a.ToBuffer();
    UnsignedTransaction b(data);

    ASSERT_EQ(a, b);
    //TODO:
    // - check timestamp
    // - check inputs
    // - check outputs
  }
}