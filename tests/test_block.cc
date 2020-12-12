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

    TEST(TestBlock, test_serialization){
        BlockPtr a = Block::Genesis();

        Buffer buff(a->GetBufferSize());
        ASSERT_TRUE(a->Encode(&buff));

        BlockPtr b = Block::NewInstance(&buff);
        ASSERT_EQ(a->GetHash(), b->GetHash());
    }

#define TOKEN_TEST_DATA_FILENAME "/home/tazz/CLionProjects/libtoken-ledger/test.dat"

    TEST(TestBlock, test_rw){
        BlockPtr genesis = Block::Genesis();
        BlockFileWriter writer(TOKEN_TEST_DATA_FILENAME);
        ASSERT_TRUE(writer.Write(genesis));

        BlockFileReader reader(TOKEN_TEST_DATA_FILENAME);
        BlockPtr blk = reader.Read();

        ASSERT_EQ(genesis->GetHash(), blk->GetHash());
    }
}