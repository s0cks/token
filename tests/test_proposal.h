#ifndef TOKEN_TEST_PROPOSAL_H
#define TOKEN_TEST_PROPOSAL_H

#include "test_suite.h"
#include "proposal.h"

namespace token{
  class RawProposalTest : public ::testing::Test{
   protected:
    RawProposalTest():
      ::testing::Test(){}
   public:
    ~RawProposalTest() override = default;
  };
}

#endif//TOKEN_TEST_PROPOSAL_H