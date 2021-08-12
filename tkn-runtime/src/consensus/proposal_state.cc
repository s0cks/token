#include "runtime.h"
#include "consensus/proposal_state.h"

namespace token{
  ProposalState::ProposalState(Runtime* runtime):
    ProposalEventListener(runtime->loop()),
    runtime_(runtime),
    phase_(Phase::kQueuedPhase),
    active_(false),
    height_(0),//TODO: use HEAD+1
    mutex_(),
    proposal_id_(),
    proposal_(nullptr),
    proposer_id_(),
    proposer_(nullptr){
    runtime->AddProposalListener(this);
  }

  void ProposalState::HandleOnProposalStart(){

  }

  void ProposalState::HandleOnProposalPrepare(){
    SetPhase(Phase::kPreparePhase);
  }

  void ProposalState::HandleOnProposalCommit(){
    SetPhase(Phase::kCommitPhase);
  }

  void ProposalState::HandleOnProposalAccepted(){}
  void ProposalState::HandleOnProposalRejected(){}
  void ProposalState::HandleOnProposalTimeout(){}

  void ProposalState::HandleOnProposalFailed(){

  }

  void ProposalState::HandleOnProposalFinished(){
    SetPhase(Phase::kQuorumPhase);
  }
}