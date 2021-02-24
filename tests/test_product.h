#ifndef TOKEN_TEST_PRODUCT_H
#define TOKEN_TEST_PRODUCT_H

#include "product.h"
#include "test_suite.h"

namespace token{
  class ProductTest : public ::testing::Test{
   protected:
    ProductTest() = default;
   public:
    ~ProductTest() = default;
  };
}

#endif//TOKEN_TEST_PRODUCT_H