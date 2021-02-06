#include "test_suite.h"
#include "object.h"

namespace token{
  TEST(TestObjectTag, test_pos){
    ObjectTag t1(Type::kBlock, 30000);
    ASSERT_TRUE(t1.IsValid());
    ASSERT_EQ(t1, t1);

    ObjectTag t2(Type::kBlock, 30000);
    ASSERT_TRUE(t2.IsValid());
    ASSERT_EQ(t1, t2);

    ObjectTag t3(Type::kBlock, 1);
    ASSERT_TRUE(t3.IsValid());
    ASSERT_NE(t1, t3);
    ASSERT_NE(t2, t3);

    ObjectTag t4(Type::kTransaction, 1);
    ASSERT_TRUE(t4.IsValid());
    ASSERT_NE(t1, t4);
    ASSERT_NE(t2, t4);
    ASSERT_NE(t3, t4);
  }
}