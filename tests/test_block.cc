#include "test_block.h"

namespace token{
  const std::string BlockTest::kExpectedHash = "C6B1B0AB8F039D212EF3E0510E2960BCE1B423AC4EDF13C3613F7B821792C306";

  TEST_BINARY_OBJECT_HASH(Block, kExpectedHash);
  TEST_BINARY_OBJECT_WRITE(Block);
  TEST_BINARY_OBJECT_EQUALS(Block);
}