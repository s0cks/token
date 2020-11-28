#include "test_suite.h"
#include "block.h"

namespace Token{
    TEST(TestBlock, test_hash){
        Handle<Block> blk = Block::Genesis();
        ASSERT_EQ(blk->GetHash(), Hash::FromHexString("9B4D1255BAC069FDBCE76E9D0DF77B1AB85C6972706402944B9E9F0655DF8C5F"));
    }

    TEST(TestBlock, test_eq){
        Handle<Block> a = Block::Genesis();
        Handle<Block> b = Block::Genesis();
        ASSERT_TRUE(a == b);
    }
}