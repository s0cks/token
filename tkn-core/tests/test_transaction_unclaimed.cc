#include <gtest/gtest.h>
#include "transaction_unclaimed.h"

namespace token{
  TEST(UnclaimedTransactionTest, TestSerialization){
    auto timestamp = Clock::now();
    auto source = TransactionReference(Hash::GenerateNonce(), 13212);
    auto user = User("VenueA");
    auto product = Product("TestToken");

    auto a = UnclaimedTransaction::NewInstance(timestamp, source, user, product);
    auto data = a->ToBuffer();
    auto b = UnclaimedTransaction::From(data);

    auto h1 = a->hash();
    auto h2 = a->hash();


    ASSERT_EQ(h1, h2);

    ASSERT_EQ(a->hash(), a->hash());
    ASSERT_EQ(b->source(), source);
    ASSERT_EQ(b->user(), user);
    ASSERT_EQ(b->product(), product);
  }
}