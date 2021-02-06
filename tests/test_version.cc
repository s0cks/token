#include "test_suite.h"
#include "version.h"

namespace token{
  TEST(TestVersion, test_eq){
    Version a(10, 10, 10);
    ASSERT_TRUE(a.valid());
    ASSERT_EQ(a.magic(), TOKEN_MAGIC);
    ASSERT_EQ(a, a);

    Version b(10, 10, 10);
    ASSERT_TRUE(b.valid());
    ASSERT_EQ(b.magic(), TOKEN_MAGIC);

    ASSERT_EQ(b, b);
    ASSERT_EQ(b, a);
  }

  TEST(TestVersion, test_neq){
    Version a(10, 10, 10);
    ASSERT_TRUE(a.valid());
    ASSERT_EQ(a.magic(), TOKEN_MAGIC);

    Version b(11, 10, 10);
    ASSERT_TRUE(b.valid());
    ASSERT_EQ(b.magic(), TOKEN_MAGIC);

    ASSERT_NE(a, b);
  }

  TEST(TestVersion, test_compare){
    Version a(1, 0, 0);
    ASSERT_TRUE(a.valid());
    ASSERT_EQ(a.magic(), TOKEN_MAGIC);

    Version b(1, 1, 0);
    ASSERT_TRUE(b.valid());
    ASSERT_EQ(b.magic(), TOKEN_MAGIC);

    Version c(1, 0, 1);
    ASSERT_TRUE(c.valid());
    ASSERT_EQ(c.magic(), TOKEN_MAGIC);

    Version d(2, 0, 0);
    ASSERT_TRUE(d.valid());
    ASSERT_EQ(d.magic(), TOKEN_MAGIC);

    ASSERT_LT(a, b);
    ASSERT_LT(a, c);
    ASSERT_LT(a, d);

    ASSERT_GT(b, a);
    ASSERT_GT(b, c);
    ASSERT_LT(b, d);

    ASSERT_LT(c, d);
    ASSERT_LT(c, b);
    ASSERT_GT(c, a);

    ASSERT_GT(d, a);
    ASSERT_GT(d, b);
    ASSERT_GT(d, c);
  }
}