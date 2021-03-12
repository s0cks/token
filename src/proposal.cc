#include "pool.h"
#include "miner.h"
#include "proposal.h"
#include "job/scheduler.h"
#include "peer/peer_session_manager.h"


namespace token{
  //TODO: refactor
#define PROPOSAL_LOG(LevelName) \
  LOG(LevelName) << "[Proposal] "

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
    Proposal* proposal = (Proposal*)handle->data;
    RpcSession* session = proposal->GetSession();

    if(!proposal->TransitionToPhase(ProposalPhase::kPreparePhase))
      return session->SendRejected();
    return session->SendAccepted();
  }

  void Proposal::OnPromise(uv_async_t* handle){
    Proposal* proposal = (Proposal*)handle->data;
    RpcSession* session = proposal->GetSession();

    Phase1Quorum& quorum = proposal->GetPhase1Quorum();
    quorum.PromiseProposal(session->GetUUID()); //TODO: check for duplicate results
  }

  void Proposal::OnCommit(uv_async_t* handle){
    Proposal* proposal = (Proposal*)handle->data;
    RpcSession* session = proposal->GetSession();

    if(!proposal->TransitionToPhase(ProposalPhase::kCommitPhase))
      return session->SendRejected();
    return session->SendAccepted();
  }

  void Proposal::OnAccepted(uv_async_t* handle){

  }

  void Proposal::OnRejected(uv_async_t* handle){

  }
}