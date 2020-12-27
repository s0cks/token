#include "test_suite.h"
#include "block.h"

namespace Token{
    TEST(TestBlock, test_hash){
        BlockPtr genesis = Block::Genesis();
        ASSERT_EQ(genesis->GetHash(), Hash::FromHexString("636777AFCC8DB5AC96371F24CDC0C9E5533665D3701AB199F637CBB2F12C10D3"));
    }

    TEST(TestBlock, test_eq){
        BlockPtr a = Block::Genesis();
        BlockPtr b = Block::Genesis();
        ASSERT_TRUE(a->GetHash() == b->GetHash());
    }
}