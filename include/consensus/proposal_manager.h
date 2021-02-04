#ifndef TOKEN_PROPOSAL_MANAGER_H
#define TOKEN_PROPOSAL_MANAGER_H

#include "consensus/proposal.h"

namespace token{
  class ProposalManager{
   private:
    ProposalManager() = delete;
   public:
    ~ProposalManager() = delete;

    static bool HasProposal();
    static bool ClearProposal();
    static bool SetProposal(const ProposalPtr& proposal);
    static bool IsProposalFor(const ProposalPtr& proposal);
    static ProposalPtr GetProposal();
  };
}

#endif//TOKEN_PROPOSAL_MANAGER_H