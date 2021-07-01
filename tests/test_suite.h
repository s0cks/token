#ifndef TOKEN_TEST_SUITE_H
#define TOKEN_TEST_SUITE_H

#include <glog/logging.h>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "transaction.h"

namespace token{
  template<class T>
  class BinaryObjectTest : public ::testing::Test{
   protected:
    BinaryObjectTest() = default;
    virtual ~BinaryObjectTest() = default;
    virtual std::shared_ptr<T> GetObject() const = 0;
    virtual std::shared_ptr<T> GetRandomObject() const = 0;
  };

#define TEST_BINARY_OBJECT_EQUALS(Name) \
  TEST_F(Name##Test, test_equals){      \
    Name##Ptr a = GetObject();          \
    Name##Ptr b = GetObject();          \
    ASSERT_TRUE(a->Equals(b));          \
  }

#define TEST_BINARY_OBJECT_HASH(Name, ExpectedHash) \
  TEST_F(Name##Test, test_hash){                    \
    Name##Ptr a = GetObject();                      \
    Hash hash = Hash::FromHexString((ExpectedHash));\
    ASSERT_EQ(a->GetHash(), hash);                  \
  }

#define TEST_BINARY_OBJECT_WRITE(Name) \
  TEST_F(Name##Test, test_write){      \
    Name##Ptr a = GetObject(); \
    BufferPtr buff = Buffer::NewInstance(a->GetBufferSize()); \
    ASSERT_TRUE(a->Write(buff));     \
    Name##Ptr b = Name::FromBytes(buff);                    \
    ASSERT_TRUE(a->Equals(b));         \
  }

  static inline TransactionPtr
  CreateRandomTransaction(const int64_t& ninputs, const int64_t& noutputs){
    int64_t idx;

    InputList inputs;
    for(idx = 0; idx < ninputs; idx++)
      inputs.push_back(Input(Hash::GenerateNonce(), idx, User("TestUser")));

    OutputList outputs;
    for(idx = 0; idx < noutputs; idx++)
      outputs.push_back(Output("TestUser", "TestToken"));

    return Transaction::NewInstance(inputs, outputs);
  }

  class IntegrationTest : public ::testing::Test{
   protected:
    IntegrationTest():
      ::testing::Test(){}

    static inline std::string
    GenerateRandomTestDirectory(const char* prefix){
      size_t length = 13 + strlen(prefix);
      char tmp_filename[length];
      snprintf(tmp_filename, length, "/tmp/%s.XXXXXX", prefix);
      return std::string(mkdtemp(tmp_filename));
    }
   public:
    virtual ~IntegrationTest() = default;
  };

  MATCHER(AnyTransaction, "Is not a transaction"){
    return arg->GetType() == Type::kTransaction;
  }

  MATCHER_P(IsHash, hash, "Hashes don't match"){
    return arg == hash;
  }

  MATCHER_P(IsBlock, hash, "Blocks don't match"){
    return arg->GetHash() == hash;
  }

  MATCHER_P(IsTransaction, tx, "Transactions don't match"){
    return arg->Equals(tx);
  }

  MATCHER_P(IsUser, expected, "The user doesn't match"){
    return arg == User(expected);
  }

  template<class M>
  class MessageTest : public ::testing::Test{
   protected:
    typedef std::shared_ptr<M> MessageTypePtr;

    MessageTest() = default;
    virtual MessageTypePtr CreateMessage() const = 0;
   public:
    virtual ~MessageTest() = default;
  };

  template<class M>
  static inline ::testing::AssertionResult
  MessagesAreEqual(const std::shared_ptr<M>& a, const std::shared_ptr<M>& b){
    if(a->Equals(b))
      return ::testing::AssertionSuccess() << "messages are equal";
    return ::testing::AssertionFailure() << "messages aren't equal";
  }

  MATCHER(IsVersionMessage, "Message is not a VersionMessage"){
    return arg->type() == Type::kVersionMessage;
  }

  MATCHER(IsVerackMessage, "Message is not a VerackMessage"){
    return arg->GetType() == Type::kVerackMessage;
  }
}

#endif //TOKEN_TEST_SUITE_H