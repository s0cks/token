#ifndef TKN_PROPOSAL_SCOPE_H
#define TKN_PROPOSAL_SCOPE_H

#include "proposal.h"

namespace token{
  namespace node{
    class Session;
  }

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

    static bool IsValidNewProposal(Proposal* proposal);
    static bool IsValidProposal(Proposal* proposal);

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

    static node::Session* GetProposer();
    static void SetProposer(node::Session* session);

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

    static inline bool
    WasPromised(){
      return GetPromises() > GetRejected();
    }

    static inline bool
    WasAccepted(){
      return GetAccepted() > GetRejected();
    }

    static inline bool
    WasRejected(){
      switch(GetPhase()){
        case ProposalPhase::kPhase1:
          return GetRejected() > GetPromises();
        case ProposalPhase::kPhase2:
          return GetRejected() > GetAccepted();
        case ProposalPhase::kQuorum:
        default:
          return false;
      }
    }
  };
}

#endif//TKN_PROPOSAL_SCOPE_H