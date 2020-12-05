#include "test_suite.h"
#include "block.h"

namespace Token{
    TEST(TestBlock, test_hash){
        BlockPtr genesis = Block::Genesis();
        ASSERT_EQ(genesis->GetHash(), Hash::FromHexString("9B4D1255BAC069FDBCE76E9D0DF77B1AB85C6972706402944B9E9F0655DF8C5F"));
    }

    TEST(TestBlock, test_eq){
        BlockPtr a = Block::Genesis();
        BlockPtr b = Block::Genesis();
        ASSERT_TRUE(a->GetHash() == b->GetHash());
    }
}