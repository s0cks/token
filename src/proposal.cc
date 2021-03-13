#include "pool.h"
#include "miner.h"
#include "job/scheduler.h"
#include "consensus/proposal.h"
#include "peer/peer_session_manager.h"

namespace token{
#define CANNOT_TRANSITION_TO(To) \
  LOG(ERROR) << "cannot transition " << raw() << " from " << GetPhase() << " phase to: " << (To) << " phase.";

#define FOR_EACH_PROPOSAL_PHASE_TRANSITIONS(V) \
  V(Queued, Prepare)                          \
  V(Prepare, Commit)

  bool Proposal::TransitionToPhase(const ProposalPhase& phase){
    LOG(INFO) << "transitioning " << raw() << " to phase: " << phase;
    switch(phase){
#define DEFINE_TRANSITION(From, To) \
      case ProposalPhase::k##To##Phase:{ \
        if(!In##From##Phase()){     \
          CANNOT_TRANSITION_TO(phase);     \
          return false;             \
        } \
        break;                      \
      }
      FOR_EACH_PROPOSAL_PHASE_TRANSITIONS(DEFINE_TRANSITION)
#undef DEFINE_TRANSITION
      default:{
        CANNOT_TRANSITION_TO(phase);
        return false;
      }
    }

    SetPhase(phase);
    return true;
  }
#undef FOR_EACH_PROPOSAL_PHASE_TRANSITIONS

  void Proposal::OnPrepare(uv_async_t* handle){
    NOT_IMPLEMENTED(WARNING);
  }

  void Proposal::OnPromise(uv_async_t* handle){
    Proposal* proposal = (Proposal*)handle->data;
    Phase1Quorum& p1quorum = proposal->GetPhase1Quorum();
    p1quorum.PromiseProposal(UUID()); //TODO: fix UUID()
 }

  void Proposal::OnCommit(uv_async_t* handle){
    NOT_IMPLEMENTED(WARNING);
  }

  void Proposal::OnAccepted(uv_async_t* handle){
    NOT_IMPLEMENTED(WARNING);
  }

  void Proposal::OnRejected(uv_async_t* handle){
    NOT_IMPLEMENTED(WARNING);
  }
}