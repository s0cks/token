#include "test_suite.h"
#include "transaction.h"

namespace Token{
    static inline TransactionPtr
    CreateTransaction(int64_t index, int64_t num_outputs, Timestamp timestamp=GetCurrentTimestamp()){
        InputList inputs;
        OutputList outputs;
        for(int64_t idx = 0; idx < num_outputs; idx++){
            std::stringstream ss;
            ss << "TestToken" << idx;
            outputs.push_back(Output("TestUser", ss.str()));
        }
        return std::make_shared<Transaction>(timestamp, index, inputs, outputs);
    }

    TEST(TestTransaction, test_hash){
        TransactionPtr tx = CreateTransaction(0, 1, 0);
        ASSERT_EQ(tx->GetHash(), Hash::FromHexString("B16C82738D9EBE7BAE78828058F68AA26DF5200BFC843DE9412887E6B082746C"));
    }

    TEST(TestTransaction, test_eq){
        TransactionPtr a = CreateTransaction(0, 3);
        TransactionPtr b = CreateTransaction(0, 3);
        ASSERT_TRUE(a->GetHash() == b->GetHash());
        TransactionPtr c = CreateTransaction(1, 3);
        ASSERT_FALSE(a->GetHash() == c->GetHash());
        TransactionPtr d = CreateTransaction(0, 1);
        ASSERT_FALSE(a->GetHash() == d->GetHash());
        ASSERT_FALSE(d->GetHash() == c->GetHash());
    }
}