#include "test_suite.h"
#include "buffer.h"
#include "block.h"

namespace Token{
    TEST(TestBuffer, test_serialization){
        Handle<Block> blk1 = Block::Genesis();
        Handle<Buffer> buff = blk1->ToBuffer();
        Handle<Block> blk2 = Block::NewInstance(buff);
        ASSERT_EQ(blk1, blk2);
    }
}