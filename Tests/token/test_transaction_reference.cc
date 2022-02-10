#include <gtest/gtest.h>
#include <glog/logging.h>

#include "token/buffer.h"
#include "token/transaction_reference.h"

namespace token{
 TEST(TransactionReferenceTest, TestEquality){
   TransactionReference reference(sha256::Nonce(1024), 10);
   auto data = NewBuffer(TransactionReference::kSize);
   ASSERT_TRUE(data->PutTransactionReference(reference));
   ASSERT_EQ(data->length(), TransactionReference::kSize);
   ASSERT_EQ(data->GetTransactionReference(), reference);
 }
}