#include "server.h"
#include "consensus/proposal.h"
#include "peer/peer_session_manager.h"

#include "pool.h"
#include "job/scheduler.h"

namespace token{
  //TODO: refactor
#define PROPOSAL_LOG(LevelName) \
  LOG(LevelName) << "[Proposal] "

  void Proposal::WaitForRequiredResponses(int required){
    LOG(INFO) << "waiting for " << required << " peers....";
    while(GetNumberOfVotes() < required); // spin
  }

  void Proposal::AcceptProposal(const UUID& node){
#ifdef TOKEN_DEBUG
    PROPOSAL_LOG(INFO) << "proposal accepted by: " << node;
#endif//TOKEN_DEBUG
    accepted_ += 1;
  }

  void Proposal::RejectProposal(const UUID& node){
#ifdef TOKEN_DEBUG
    PROPOSAL_LOG(INFO) << "proposal rejected by: " << node;
#endif//TOKEN_DEBUG
    rejected_ += 1;
  }

  int Proposal::GetRequiredNumberOfPeers(){
    #ifdef TOKEN_ENABLE_SERVER
    int32_t peers = PeerSessionManager::GetNumberOfConnectedPeers();
    if(peers == 0){ return 0; }
    else if(peers == 1) return 1;
    return peers / 2;
    #else
    return 0;
    #endif//TOKEN_ENABLE_SERVER
  }

#define CANNOT_TRANSITION_TO(From, To) \
  LOG(ERROR) << "cannot transition " << raw() << " from " << (From) << " phase to: " << (To) << " phase.";

  bool Proposal::TransitionToPhase(const Phase& phase){
    LOG(INFO) << "transitioning " << raw() << " to phase: " << phase;
    switch(phase){
      case Proposal::kVotingPhase:{
        if(!IsProposal()){
          CANNOT_TRANSITION_TO(GetPhase(), phase);
          return false;
        }
        SetPhase(phase);
        return true;
      }
      case Proposal::kCommitPhase:{
        if(!IsVoting()){
          CANNOT_TRANSITION_TO(GetPhase(), phase);
          return false;
        }
        SetPhase(phase);
        return true;
      }
      case Proposal::kQuorumPhase:{
        if(IsQuorum()){ //TODO fix flow logic?
          CANNOT_TRANSITION_TO(GetPhase(), phase);
          return false;
        }
        SetPhase(phase);
        return true;
      }
      case Proposal::kProposalPhase:
      default:{
        CANNOT_TRANSITION_TO(GetPhase(), phase);
        return false;
      }
    }
  }

  void Proposal::OnTimeout(uv_timer_t* handle){

  }

  void Proposal::OnPrepare(uv_async_t* handle){

  }

  void Proposal::OnPromise(uv_async_t* handle){

  }

  void Proposal::OnCommit(uv_async_t* handle){

  }

  void Proposal::OnAccepted(uv_async_t* handle){

  }

  void Proposal::OnRejected(uv_async_t* handle){

  }
}