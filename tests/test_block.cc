#include "test_suite.h"
#include "block.h"

namespace Token{
    TEST(TestBlock, test_hash){
        BlockPtr genesis = Block::Genesis();
        ASSERT_EQ(genesis->GetHash(), Hash::FromHexString("28C4AD5C28971E661C770253EF4F3FE9F3881394168140CF26F86C3B0C5B8C8A"));
    }

    TEST(TestBlock, test_eq){
        BlockPtr a = Block::Genesis();
        BlockPtr b = Block::Genesis();
        ASSERT_TRUE(a->GetHash() == b->GetHash());
    }
}