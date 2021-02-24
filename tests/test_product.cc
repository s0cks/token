#include "test_product.h"

namespace token{
  TEST_F(ProductTest, test_eq1){
    Product a = Product("Product1");
    ASSERT_EQ(a, a);
  }

  TEST_F(ProductTest, test_eq2){
    Product a = Product("Product1");
    Product b = Product("Product1");
    ASSERT_EQ(a, b);
  }
}