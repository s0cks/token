#include "test_suite.h"
#include "unclaimed_transaction.h"

namespace Token{
    TEST(TestUnclaimedTransaction, test_eq){
        UnclaimedTransactionPtr a = std::make_shared<UnclaimedTransaction>(Hash(), 0, "TestUser", "TestToken");
        UnclaimedTransactionPtr b = std::make_shared<UnclaimedTransaction>(Hash(), 0, "TestUser", "TestToken2");

        ASSERT_EQ(a->GetHash(), a->GetHash());
        ASSERT_NE(a->GetHash(), b->GetHash());
    }

    TEST(TestUnclaimedTransaction, test_serialization){
        UnclaimedTransactionPtr a = std::make_shared<UnclaimedTransaction>(Hash(), 1, "TestUser", "TestToken");

        Buffer buff(a->GetBufferSize());
        ASSERT_TRUE(a->Encode(&buff));

        UnclaimedTransactionPtr b = UnclaimedTransaction::NewInstance(&buff);
        ASSERT_EQ(a->GetHash(), b->GetHash());
    }
}