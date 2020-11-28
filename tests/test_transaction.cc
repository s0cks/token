#include "test_suite.h"
#include "token.h"

#include "transaction.h"

namespace Token{
    static inline Handle<Transaction>
    CreateTransaction(int64_t index, int64_t num_outputs, Timestamp timestamp=GetCurrentTimestamp()){
        Input* inputs[0] = {};
        Output** outputs = new Output*[num_outputs];
        for(int64_t idx = 0; idx < num_outputs; idx++){
            std::stringstream ss;
            ss << "TestToken" << idx;
            outputs[idx] = Output::NewInstance("TestUser", ss.str());
        }
        return Transaction::NewInstance(index, inputs, 0, outputs, num_outputs, timestamp);
    }

    TEST(TestTransaction, test_hash){
        Handle<Transaction> tx = CreateTransaction(0, 1, 0);
        ASSERT_EQ(tx->GetHash(), Hash::FromHexString("B16C82738D9EBE7BAE78828058F68AA26DF5200BFC843DE9412887E6B082746C"));
    }

    TEST(TestTransaction, test_eq){
        Handle<Transaction> a = CreateTransaction(0, 3);
        Handle<Transaction> b = CreateTransaction(0, 3);
        ASSERT_TRUE(a == b);
        Handle<Transaction> c = CreateTransaction(1, 3);
        ASSERT_FALSE(a == c);
        Handle<Transaction> d = CreateTransaction(0, 1);
        ASSERT_FALSE(a == d);
        ASSERT_FALSE(d == c);
    }
}