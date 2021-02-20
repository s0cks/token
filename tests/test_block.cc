#include "test_suite.h"
#include "block.h"
#include "block_file.h"

namespace token{
  static inline BlockPtr
  CreateA(){
    // predictable
    return Block::Genesis();
  }

  static inline BlockPtr
  CreateB(){
    // unpredictable
    BlockPtr genesis = Block::Genesis();
    return Block::FromParent(genesis, {});
  }

  DEFINE_BINARY_OBJECT_POSITIVE_TEST(Block, CreateA);
  DEFINE_BINARY_OBJECT_NEGATIVE_TEST(Block, CreateA, CreateB);
  DEFINE_BINARY_OBJECT_HASH_TEST(Block, CreateA, "1191F9F9657BB3AB6D6E043D7B4017507D802795FE58EFA3043A066980C32C72");

  TEST(TestBlock, test_serialization){
    BlockPtr blk1 = Block::Genesis();
    ASSERT_NE(blk1, nullptr);

    FILE* file = tmpfile();
    ASSERT_NE(file, nullptr);
    ASSERT_TRUE(WriteBlock(file, blk1));
    ASSERT_TRUE(Flush(file));
    ASSERT_TRUE(Seek(file, 0));

    BlockPtr blk2 = ReadBlock(file);
    ASSERT_NE(blk2, nullptr);
    ASSERT_TRUE(blk1->Equals(blk2));
  }
}