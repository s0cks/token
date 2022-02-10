#include <gtest/gtest.h>
#include "token/product.h"
#include "token/buffer.h"

#include "helpers.h"

namespace token{
 class ProductTest : public ::testing::Test{
  public:
   ProductTest() = default;
   ~ProductTest() override = default;

   static constexpr const char* kTestName = "TestProduct";
 };

 TEST_F(ProductTest, TestEquality){
   Product product(kTestName);
   ASSERT_TRUE(IsProduct(product, kTestName));
   ASSERT_FALSE(IsProduct(product, "Test"));
 }

 TEST_F(ProductTest, TestSerialization){
   Product product(kTestName);
   auto data = NewBuffer(Product::kSize);
   ASSERT_TRUE(data->PutProduct(product));
   ASSERT_EQ(data->GetProduct(), product);
 }
}