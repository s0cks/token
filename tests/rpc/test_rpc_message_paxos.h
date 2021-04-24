#ifndef TOKEN_TEST_RPC_MESSAGE_PAXOS_H
#define TOKEN_TEST_RPC_MESSAGE_PAXOS_H

#include "test_suite.h"
#include "rpc/rpc_message.h"

namespace token{
  class PaxosMessageTest : public ::testing::Test{
   public:
    static inline RawProposal
    CreateProposalFor(const BlockPtr& blk){
      return RawProposal(Clock::now(), UUID(), UUID(), blk->GetHeader());
    }

    static inline RawProposal
    CreateProposalForGenesis(){
      return CreateProposalFor(Block::Genesis());
    }

    static inline RawProposal
    CreateRandomProposal(){
      return CreateProposalForGenesis();//TODO: fixme
    }
   protected:
    PaxosMessageTest():
      ::testing::Test(){}
   public:
    ~PaxosMessageTest() override = default;
  };

#define DECLARE_PAXOS_MESSAGE_TEST(Name) \
  class Name##MessageTest : public PaxosMessageTest{ \
    protected:                           \
      Name##MessageTest():               \
        PaxosMessageTest(){}             \
    public:                              \
      ~Name##MessageTest() override = default;       \
  };
  FOR_EACH_PAXOS_MESSAGE_TYPE(DECLARE_PAXOS_MESSAGE_TEST)
#undef DECLARE_PAXOS_MESSAGE_TEST
}

#endif//TOKEN_TEST_RPC_MESSAGE_PAXOS_H