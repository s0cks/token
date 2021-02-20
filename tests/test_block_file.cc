#include "block.h"
#include "test_block_file.h"

namespace token{
  TEST_F(BlockFileTest, test_rw){
    BlockPtr a = Block::Genesis();
    ASSERT_TRUE(a);
    ASSERT_TRUE(WriteBlock(file_, a));
    ASSERT_TRUE(Seek(file_, 0));
    BlockPtr b = ReadBlock(file_);
    ASSERT_TRUE(b);
    ASSERT_TRUE(a->Equals(b));
  }
}