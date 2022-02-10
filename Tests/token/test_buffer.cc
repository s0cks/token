#include <gtest/gtest.h>
#include <glog/logging.h>

#include "token/buffer.h"

namespace token{
 class StackBufferTest : public ::testing::Test{
  public:
   StackBufferTest() = default;
   ~StackBufferTest() override = default;
 };

 class HeapBufferTest : public ::testing::Test{
  public:
   HeapBufferTest() = default;
   ~HeapBufferTest() override = default;
 };

 static constexpr const Byte kTestByteValue = 3;
 static constexpr const UnsignedByte kTestUnsignedByteValue = -3;

 static constexpr const Short kTestShortValue = 12893;
 static constexpr const UnsignedShort kTestUnsignedShortValue = -484;

 static constexpr const Int kTestIntValue = 65536;
 static constexpr const UnsignedInt kTestUnsignedIntValue = -4844;

 static constexpr const Long kTestLongValue = 394844;
 static constexpr const UnsignedLong kTestUnsignedLongValue = -37844;

#define DEFINE_BUFFER_SIGNED_TYPE_TEST_CASE(Buffer, Name) \
 TEST_F(Buffer##Test, Test##Name){                        \
  auto data = NewBuffer(sizeof(Name));                    \
  ASSERT_TRUE(data->Put##Name(kTest##Name##Value));       \
  ASSERT_EQ(data->length(), sizeof(Name));                \
  ASSERT_EQ(data->Get##Name(), kTest##Name##Value);       \
 }

#define DEFINE_BUFFER_UNSIGNED_TYPE_TEST_CASE(Buffer, Name) \
 TEST_F(Buffer##Test, TestUnsigned##Name){                  \
  auto data = NewBuffer(sizeof(Unsigned##Name));            \
  ASSERT_TRUE(data->PutUnsigned##Name(kTestUnsigned##Name##Value)); \
  ASSERT_EQ(data->length(), sizeof(Unsigned##Name));        \
  ASSERT_EQ(data->GetUnsigned##Name(), kTestUnsigned##Name##Value); \
 }

 TEST_F(HeapBufferTest, TestHash){
   auto val = sha256::Nonce(1024);
   auto data = NewBuffer(uint256::kSize);
   ASSERT_TRUE(data->PutSHA256(val));
   ASSERT_EQ(data->length(), sha256::kSize);
   ASSERT_EQ(data->GetSHA256(), val);
 }

#define DEFINE_STACK_BUFFER_TYPE_TEST_CASE(Name) \
 DEFINE_BUFFER_SIGNED_TYPE_TEST_CASE(StackBuffer, Name) \
 DEFINE_BUFFER_UNSIGNED_TYPE_TEST_CASE(StackBuffer, Name)

#define DEFINE_HEAP_BUFFER_TYPE_TEST_CASE(Name) \
 DEFINE_BUFFER_SIGNED_TYPE_TEST_CASE(HeapBuffer, Name) \
 DEFINE_BUFFER_UNSIGNED_TYPE_TEST_CASE(HeapBuffer, Name)

 FOR_EACH_BUFFER_TYPE(DEFINE_STACK_BUFFER_TYPE_TEST_CASE)
 FOR_EACH_BUFFER_TYPE(DEFINE_HEAP_BUFFER_TYPE_TEST_CASE)
}