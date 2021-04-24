#include "test_proposal.h"

namespace token{
  TEST_F(RawProposalTest, EqualsTest){
    RawProposal proposal = RawProposal(Clock::now(), UUID(), UUID(), Block::Genesis()->GetHeader());
    ASSERT_EQ(proposal, proposal) << "proposals aren't equal";
  }

  TEST_F(RawProposalTest, WriteTest){
    RawProposal proposal = RawProposal(Clock::now(), UUID(), UUID(), Block::Genesis()->GetHeader());
    BufferPtr tmp = Buffer::NewInstance(proposal.GetBufferSize());
    ASSERT_TRUE(proposal.Write(tmp)) << "cannot write proposal to tmp buffer";
    ASSERT_EQ(tmp->GetWrittenBytes(), proposal.GetBufferSize()) << "not the right amount of bytes for proposal";
    ASSERT_EQ(proposal, RawProposal(tmp));
  }
}