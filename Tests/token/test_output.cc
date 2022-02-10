#include <gtest/gtest.h>
#include <glog/logging.h>

#include "helpers.h"
#include "token/output.h"

namespace token{
 class OutputTest : public ::testing::Test{
  public:
   OutputTest() = default;
   ~OutputTest() override = default;

   static constexpr const char* kTestUser = "TestUser";
   static constexpr const char* kTestProduct = "TestProduct";
 };

 TEST_F(OutputTest, TestEquality){
   Output output(kTestUser, kTestProduct);
   ASSERT_TRUE(IsOutput(output, kTestUser, kTestProduct));
 }

 TEST_F(OutputTest, TestSerialization){
   Output a(kTestUser, kTestProduct);
   auto data = NewBuffer(Output::kSize);
   ASSERT_TRUE(a.WriteTo(data));
   ASSERT_EQ(data->length(), Output::kSize);

   Output b(data);
   ASSERT_TRUE(IsOutput(b, kTestUser, kTestProduct));
 }
}