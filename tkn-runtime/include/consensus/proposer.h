#ifndef TKN_PROPOSER_H
#define TKN_PROPOSER_H

#include <uv.h>
#include "proposal.h"
#include "miner_listener.h"
#include "election.h"
#include "proposal_listener.h"

namespace token{
#define FOR_EACH_PROPOSER_STATE(V) \
  V(Queued)                        \
  V(Prepare)                       \
  V(Commit)                        \
  V(Quorum)

  class Runtime;
  class Proposer : public ElectionEventListener,
                   public MinerEventListener{
  protected:
    Runtime* runtime_;
    Election prepare_;
    Election commit_;

    bool StartProposal();
    bool EndProposal();
    bool ClearProposal();

    DEFINE_MINER_EVENT_LISTENER;
    DEFINE_ELECTION_EVENT_LISTENER;
  public:
    explicit Proposer(Runtime* runtime);
    ~Proposer() override = default;

    Runtime* GetRuntime() const{
      return runtime_;
    }

    Election& GetElectionForPhase(const ProposalState::Phase& phase){
      switch(phase){
        case ProposalState::Phase::kPreparePhase:
          return prepare_;
        case ProposalState::Phase::kCommitPhase:
          return commit_;
        default:
          DLOG(FATAL) << "unknown phase: " << phase;
      }
    }

    Election& GetPrepareElection(){
      return prepare_;
    }

    Election& GetCommitElection(){
      return commit_;
    }
  };
}

#endif//TKN_PROPOSER_H