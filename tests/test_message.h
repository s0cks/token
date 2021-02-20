#ifndef TOKEN_TEST_MESSAGE_H
#define TOKEN_TEST_MESSAGE_H

#include "test_suite.h"
#include "rpc/rpc_message.h"

namespace token{
  template<class M>
  class MessageTest : public ::testing::Test{
   protected:
    MessageTest() = default;
    virtual std::shared_ptr<M> CreateMessage() const = 0;
   public:
    virtual ~MessageTest() = default;
  };

#define DEFINE_MESSAGE_TESTCASE(Name) \
  class Name##MessageTest : public MessageTest<Name##Message>{ \
    protected:                        \
      Name##MessageTest() = default;  \
      Name##MessagePtr CreateMessage() const;                  \
    public:                           \
      ~Name##MessageTest() = default; \
  };
  FOR_EACH_MESSAGE_TYPE(DEFINE_MESSAGE_TESTCASE)
#undef DEFINE_MESSAGE_TESTCASE
}

#endif//TOKEN_TEST_MESSAGE_H