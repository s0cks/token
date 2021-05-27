#include "test_block_header.h"

#include "block.h"

namespace token{
  TEST_F(BlockHeaderTest, EqualsTest){
    BlockHeader a = Block::Genesis()->GetHeader();
    ASSERT_EQ(a, Block::Genesis()->GetHeader()) << "headers aren't equal";
  }

  TEST_F(BlockHeaderTest, WriteTest){
    BlockHeader a = Block::Genesis()->GetHeader();
    BufferPtr tmp = Buffer::NewInstance(a.GetBufferSize());
    ASSERT_TRUE(a.Write(tmp)) << "cannot write message to tmp buffer";
    ASSERT_EQ(tmp->GetWrittenBytes(), a.GetBufferSize()) << "wrong amount of written bytes";
    BlockHeader b(tmp);
    ASSERT_EQ(a, b) << "headers aren't equal";
  }
}