#include "test_block.h"

namespace token{
  const std::string BlockTest::kExpectedHash = "AC9A79E04E4E280D56730CE4FF5B3D56EC61B3C7C93DDBD03A53479BAF45A2D7";

  TEST_BINARY_OBJECT_HASH(Block, kExpectedHash);
  TEST_BINARY_OBJECT_WRITE(Block);
  TEST_BINARY_OBJECT_EQUALS(Block);
}