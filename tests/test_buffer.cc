#include "test_suite.h"
#include "buffer.h"
#include "block.h"

namespace Token{
    TEST(TestBuffer, test_serialization){
        BlockPtr blk1 = Block::Genesis();

        Buffer* buff = blk1->ToBuffer();
        BlockPtr blk2 = std::make_shared<Block>(buff);

        ASSERT_TRUE(blk1->GetHash() == blk2->GetHash());
    }
}