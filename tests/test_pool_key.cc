#include "test_suite.h"
#include "pool.h"

namespace Token{
    TEST(TestObjectPoolKey, test_eq){
        ObjectTag t1(ObjectTag::kBlock);
        Hash h1 = Hash::GenerateNonce();
        ObjectPoolKey k1(t1, h1);
        ObjectPoolKey k2(t1, h1);

        ASSERT_EQ(k1, k1);
        ASSERT_EQ(k2, k2);
        ASSERT_EQ(k1, k2);

        ObjectTag t2(ObjectTag::kTransaction);
        Hash h2 = Hash::GenerateNonce();
        ObjectPoolKey k3(t2, h2);
        ASSERT_EQ(k3, k3);
        ASSERT_NE(k3, k1);
        ASSERT_NE(k3, k2);
    }

    TEST(TestObjectPoolKey, test_ne){
        ObjectPoolKey k1(ObjectTag(ObjectTag::kBlock), Hash::GenerateNonce());
        ObjectPoolKey k2(ObjectTag(ObjectTag::kBlock), Hash::GenerateNonce());
        ASSERT_NE(k1, k2);

        Hash h = Hash::GenerateNonce();
        ObjectPoolKey k3(ObjectTag(ObjectTag::kBlock), h);
        ObjectPoolKey k4(ObjectTag(ObjectTag::kTransaction), h);
        ASSERT_NE(k3, k1);
        ASSERT_NE(k3, k2);
        ASSERT_NE(k3, k4);
    }
}