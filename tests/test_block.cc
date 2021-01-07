#include "test_suite.h"
#include "block.h"

namespace Token{
  static inline BlockPtr
  CreateA(){
    // predictable
    return Block::Genesis();
  }

  static inline BlockPtr
  CreateB(){
    // unpredictable
    BlockPtr genesis = Block::Genesis();
    return Block::NewInstance(genesis, {});
  }

  DEFINE_BINARY_OBJECT_POSITIVE_TEST(Block, CreateA);
  DEFINE_BINARY_OBJECT_NEGATIVE_TEST(Block, CreateA, CreateB);
  DEFINE_BINARY_OBJECT_HASH_TEST(Block, CreateA, "1191F9F9657BB3AB6D6E043D7B4017507D802795FE58EFA3043A066980C32C72");
  DEFINE_BINARY_OBJECT_SERIALIZATION_TEST(Block, CreateA);
}