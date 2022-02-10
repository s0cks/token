#ifndef TOKEN_TEST_HELPERS_H
#define TOKEN_TEST_HELPERS_H

#include <gtest/gtest.h>

#include "token/user.h"
#include "token/input.h"
#include "token/output.h"
#include "token/product.h"
#include "token/transaction.h"

namespace token{
 static inline ::testing::AssertionResult
 IsUser(const User& actual, const char* expected){
   if(actual != expected)
     return ::testing::AssertionFailure() << actual << " does not equal " << expected;
   return ::testing::AssertionSuccess();
 }

 static inline ::testing::AssertionResult
 IsProduct(const Product& actual, const char* expected){
   if(actual != expected)
     return ::testing::AssertionFailure() << actual << " does not equal " << expected;
   return ::testing::AssertionSuccess();
 }

 static inline ::testing::AssertionResult
 IsOutput(const Output& actual, const char* expected_user, const char* expected_product){
   if(actual.user() != expected_user)
     return ::testing::AssertionFailure() << "Output user_ " << actual.user() << " does not equal " << expected_user;
   if(actual.product() != expected_product)
     return ::testing::AssertionFailure() << "Output product_ " << actual.product() << " does not equal " << expected_product;
   return ::testing::AssertionSuccess();
 }

 static inline ::testing::AssertionResult
 IsInput(const Input& actual, const TransactionReference& expected_source){
   if(actual.source() != expected_source)
     return ::testing::AssertionFailure() << "Input source_ " << actual.source() << " does not equal " << expected_source;
   return ::testing::AssertionSuccess();
 }

 static inline ::testing::AssertionResult
 TimestampsAreEqual(const Timestamp& lhs, const Timestamp& rhs){
   DLOG(INFO) << "lhs: " << ToUnixTimestamp(lhs);
   DLOG(INFO) << "rhs: " << ToUnixTimestamp(rhs);
   if(ToUnixTimestamp(lhs) != ToUnixTimestamp(rhs))
     return ::testing::AssertionFailure() << "Timestamp " << FormatTimestampReadable(lhs) << " does not equal " << FormatTimestampReadable(rhs);
   return ::testing::AssertionSuccess();
 }

 static inline std::string
 ToString(const Input* inputs, uint64_t num_inputs){
   std::stringstream ss;
   ss << '[';
   for(auto idx = 0; idx < num_inputs; idx++){
     ss << inputs[idx];
     if(idx < num_inputs - 1)
       ss << ',';
   }
   ss << ']';
   return ss.str();
 }

 static inline ::testing::AssertionResult
 InputsEqual(const Transaction& tx, const Input* inputs, uint64_t num_inputs){
   if(tx.GetNumberOfInputs() != num_inputs)
     return ::testing::AssertionFailure() << "Transaction was expected to have " << num_inputs << " Inputs, but had:" << tx.GetNumberOfInputs();
   auto result = std::equal(tx.inputs_begin(), tx.inputs_end(),inputs);
   if(!result){
     auto failure = ::testing::AssertionFailure() << std::endl;
     failure << "Transaction inputs:" << std::endl;
     failure << "\t" << ToString(tx.inputs_begin(), num_inputs) << std::endl;
     failure << "Where expected to equal:" << std::endl;
     failure << "\t" << ToString(inputs, num_inputs);
     return failure;
   }
   return ::testing::AssertionSuccess();
 }

 static inline ::testing::AssertionResult
 OutputsEqual(const Transaction& tx, const Output* outputs, uint64_t num_outputs){
   if(tx.GetNumberOfOutputs() != num_outputs)
     return ::testing::AssertionFailure() << "Transaction was expected to have " << num_outputs << " Outputs, but had: " << tx.GetNumberOfOutputs();
   auto result = std::equal(tx.outputs_begin(), tx.outputs_end(), outputs);
   if(!result)
     return ::testing::AssertionFailure() << "Transaction outputs do not equal: ";//TODO: make better
   return ::testing::AssertionSuccess();
 }
}

#endif //TOKEN_TEST_HELPERS_H
