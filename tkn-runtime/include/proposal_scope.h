#ifndef TKN_PROPOSAL_SCOPE_H
#define TKN_PROPOSAL_SCOPE_H

#include "proposal.h"

namespace token{
  enum class ProposalPhase{
    kQueued,
    kPhase1,
    kPhase2,
    kQuorum,
  };

  static inline std::ostream&
  operator<<(std::ostream& stream, const ProposalPhase& phase){
    NOT_IMPLEMENTED(ERROR);
    return stream << "Unknown";
  }

  class ProposalScope{//TODO: refactor
  public:
    ProposalScope() = delete;
    ~ProposalScope() = delete;

    static bool HasActiveProposal();
    static bool SetActiveProposal(const Proposal& proposal);
    static Proposal* GetCurrentProposal();
    static Proposal* CreateNewProposal(const int64_t& height, const Hash& hash);

    static void SetPhase(const ProposalPhase& phase);
    static ProposalPhase GetPhase();
    static uint64_t GetRequiredVotes();

    static uint64_t GetPromises();
    static uint64_t GetAccepted();
    static uint64_t GetRejected();

    static void Promise(const UUID& node_id);
    static void Accepted(const UUID& node_id);
    static void Rejected(const UUID& node_id);

    static inline uint64_t
    GetCurrentVotes(){
      switch(GetPhase()){
        case ProposalPhase::kPhase1:
          return GetPromises();
        case ProposalPhase::kPhase2:
          return GetAccepted() + GetRejected();
        case ProposalPhase::kQuorum:
        default:
          return 0;
      }
    }

    static inline bool
    HasRequiredVotes(){
      return GetCurrentVotes() >= GetRequiredVotes();
    }
  };
}

#endif//TKN_PROPOSAL_SCOPE_H