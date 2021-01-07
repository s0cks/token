#ifndef TOKEN_TEST_SUITE_H
#define TOKEN_TEST_SUITE_H

#include <glog/logging.h>
#include <gtest/gtest.h>

#define DEFINE_BINARY_OBJECT_POSITIVE_TEST(Name, Gen) \
  TEST(Test##Name, test_pos){                         \
    Name##Ptr a = Gen();                              \
    Name##Ptr b = Gen();                              \
    ASSERT_TRUE(a->Equals(b));                        \
  }

#define DEFINE_BINARY_OBJECT_NEGATIVE_TEST(Name, GenA, GenB) \
  TEST(Test##Name, test_neg){                                \
    Name##Ptr a = GenA();                                    \
    Name##Ptr b = GenB();                                    \
    ASSERT_FALSE(a->Equals(b));                              \
  }

#define DEFINE_BINARY_OBJECT_HASH_TEST(Name, Gen, Expected) \
  TEST(Test##Name, test_hash){                              \
    Name##Ptr obj = Gen();                                  \
    Hash hash = Hash::FromHexString((Expected));            \
    ASSERT_EQ(obj->GetHash(), hash);                        \
  }

#define DEFINE_BINARY_OBJECT_SERIALIZATION_TEST(Name, Gen) \
  TEST(Test##Name, test_serialization){                    \
    Name##Ptr a = Gen();                                   \
    BufferPtr buff = Buffer::NewInstance(a->GetBufferSize()); \
    ASSERT_TRUE(a->Write(buff));                           \
    Name##Ptr b = Name::FromBytes(buff);                   \
    ASSERT_TRUE(a->Equals(b));                             \
  }

#define DEFINE_RAW_OBJECT_POSITIVE_TEST(Name, Gen) \
  TEST(Test##Name, test_pos){                      \
    Name a = Gen();                                \
    Name b = Gen();                                \
    ASSERT_EQ(a, b);                               \
  }

#define DEFINE_RAW_OBJECT_NEGATIVE_TEST(Name, GenA, GenB) \
  TEST(Test##Name, test_neg){                             \
    Name a = GenA();                                      \
    Name b = GenB();                                      \
    ASSERT_NE(a, b);                                      \
  }

#define DEFINE_RAW_OBJECT_SERIALIZATION_TEST(Name, Gen) \
  TEST(Test##Name, test_serialization){                 \
    Name a = Gen();                                     \
    BufferPtr buff = Buffer::NewInstance(Name::kSize);  \
    ASSERT_TRUE(buff->Put##Name(a));                    \
    Name b = buff->Get##Name();                         \
  }

#endif //TOKEN_TEST_SUITE_H