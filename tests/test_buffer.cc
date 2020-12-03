#include "test_suite.h"
#include "buffer.h"
#include "block.h"

namespace Token{
    TEST(TestBuffer, test_serialization){
        Block blk1 = Block::Genesis();

        Buffer* buff = blk1.ToBuffer();
        Block blk2(buff);
        delete buff;

        ASSERT_TRUE(blk1 == blk2);
    }
}