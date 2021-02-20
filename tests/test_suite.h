#ifndef TOKEN_TEST_SUITE_H
#define TOKEN_TEST_SUITE_H

#include <glog/logging.h>
#include <gtest/gtest.h>

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
}

#endif //TOKEN_TEST_SUITE_H