#include "test_suite.h"
#include "utils/compression.h"

namespace Token{
  TEST(TestCompression, test_round_robbin){
    BufferPtr buff1 = Buffer::NewInstance(65535);
    buff1->PutString("Hello World");

    BufferPtr buff2 = Buffer::NewInstance(65535);

    ASSERT_TRUE(Compress(buff1, buff2));

    BufferPtr buff3 = Buffer::NewInstance(65535);
    ASSERT_TRUE(Decompress(buff2, buff3));
    ASSERT_EQ(buff3->GetString(11), "Hello World");
  }
}