#include "test_message.h"

namespace token{
#define DEFINE_MESSAGE_SERIALIZATION_TESTCASE(Name) \
  TEST_F(Name##MessageTest, test_serialization){    \
    Name##MessagePtr a = CreateMessage();           \
    int64_t size = a->GetBufferSize();              \
    BufferPtr tmp = Buffer::NewInstance(size);      \
    ASSERT_TRUE(a->Write(tmp));                     \
    ASSERT_EQ(tmp->GetWrittenBytes(), size);        \
    ASSERT_EQ(tmp->GetObjectTag(), ObjectTag(Type::k##Name##Message, size)); \
    Name##MessagePtr b = Name##Message::NewInstance(tmp);                    \
    ASSERT_TRUE(a->Equals(b));                      \
  }

  DEFINE_MESSAGE_SERIALIZATION_TESTCASE(Version);
  DEFINE_MESSAGE_SERIALIZATION_TESTCASE(Verack);
  DEFINE_MESSAGE_SERIALIZATION_TESTCASE(Block);
  DEFINE_MESSAGE_SERIALIZATION_TESTCASE(Transaction);
  DEFINE_MESSAGE_SERIALIZATION_TESTCASE(Prepare);
  DEFINE_MESSAGE_SERIALIZATION_TESTCASE(Promise);
  DEFINE_MESSAGE_SERIALIZATION_TESTCASE(Commit);
  DEFINE_MESSAGE_SERIALIZATION_TESTCASE(Accepted);
  DEFINE_MESSAGE_SERIALIZATION_TESTCASE(Rejected);
  DEFINE_MESSAGE_SERIALIZATION_TESTCASE(NotSupported);
#undef DEFINE_MESSAGE_SERIALIZATION_TESTCASE
}