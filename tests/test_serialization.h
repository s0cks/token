#ifndef TOKEN_TEST_SERIALIZATION_H
#define TOKEN_TEST_SERIALIZATION_H

#include "test_suite.h"
#include "filesystem.h"

namespace token{
  class TestSerialization : public ::testing::Test{
   protected:
    FILE* file_;

    void SetUp(){
      file_ = tmpfile();
      ASSERT_NE(file_, nullptr);
    }

    void TearDown(){
      ASSERT_TRUE(Close(file_));
    }
   public:
    TestSerialization() = default;
    ~TestSerialization() = default;
  };
}

#endif//TOKEN_TEST_SERIALIZATION_H