#include "test_suite.h"
#include "object_tag.h"

namespace Token{
    TEST(TestObjectTag, test_pos){
        ObjectTag tag(ObjectTag::kBlock);
        ASSERT_TRUE(tag.IsMagic());
        ASSERT_TRUE(tag.IsBlock());
    }

    TEST(TestObjectTag, test_eq){
        ObjectTag t1(ObjectTag::kBlock);
        ObjectTag t2(ObjectTag::kBlock);
        ASSERT_EQ(t1, t2);
    }

    TEST(TestObjectTag, test_ne){
        ObjectTag t1(ObjectTag::kBlock);
        ObjectTag t2(ObjectTag::kTransaction);
        ASSERT_NE(t1, t2);
    }
}