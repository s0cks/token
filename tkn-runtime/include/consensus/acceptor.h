#ifndef TKN_ACCEPTOR_H
#define TKN_ACCEPTOR_H

#include "election.h"
#include "proposal_listener.h"

namespace token{
  class Runtime;
  class Acceptor : public ProposalEventListener,
                   public ElectionEventListener{
  private:
    Runtime* runtime_;

    Election& GetCurrentElection(const ProposalState::Phase& phase) const;
    bool IsValidProposal(const UUID& proposal, const Hash& hash);
    bool CommitProposal(const UUID& proposal, const Hash& hash);
    void AcceptProposal(const ProposalState::Phase& phase, const UUID& node_id);
    void RejectProposal(const ProposalState::Phase& phase, const UUID& node_id);

    DEFINE_PROPOSAL_EVENT_LISTENER;
    DEFINE_ELECTION_EVENT_LISTENER;
  public:
    explicit Acceptor(Runtime* runtime);
    ~Acceptor() override = default;

    Runtime* GetRuntime() const{
      return runtime_;
    }
  };
}

#endif//TKN_ACCEPTOR_H