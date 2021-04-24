#include "miner.h"
#include "proposal.h"
#include "job/scheduler.h"
#include "peer/peer_session_manager.h"

namespace token{
  int16_t Proposal::GetNumberOfRequiredVotes() {
    return PeerSessionManager::GetNumberOfConnectedPeers();
  }

#define CANNOT_TRANSITION_TO(To) \
  LOG(ERROR) << "cannot transition " << raw() << " from " << GetPhase() << " phase to: " << (To) << " phase.";

  bool ProposalJob::AcceptProposal(){
    PeerSessionManager::BroadcastAccepted();//TODO: check result
    return true;
  }

  bool ProposalJob::RejectProposal(){
    PeerSessionManager::BroadcastRejected();//TODO: check result
    return false;
  }

  bool ProposalJob::CommitProposal(){
    NOT_IMPLEMENTED(ERROR);
    return true;
  }

  bool ProposalJob::PauseMiner(){
    return BlockMiner::GetInstance()->Pause();
  }

  bool ProposalJob::ResumeMiner(){
    return BlockMiner::GetInstance()->Resume();
  }

  bool ProposalJob::SendAcceptedToMiner(){
    return BlockMiner::GetInstance()->SendAccepted();
  }

  bool ProposalJob::SendRejectedToMiner(){
    return BlockMiner::GetInstance()->SendRejected();
  }

  // block is already validated, need to share w/ peers and wait for confirmation
  bool ProposalJob::ExecutePhase1(){
    ProposalPtr proposal = GetProposal();
    DLOG(INFO) << "executing phase 1 for proposal " << proposal->GetID();
    PeerSessionManager::BroadcastPrepare();
    if(!proposal->StartTimer()) {
      DLOG(ERROR) << "cancelling proposal " << proposal->GetID() << ", cannot start timer.";
      return RejectProposal();
    }

    DLOG(INFO) << "waiting for " << proposal->GetRequired() << " votes....";
    while(proposal->GetCurrentVotes() < proposal->GetRequired());//spin

    if(proposal->GetPercentageRejects() > proposal->GetPercentagePromises()){
      DLOG(WARNING) << "proposal " << proposal->GetID() << " was rejected by " << proposal->GetPercentageRejects() << "% of peers.";
      return RejectProposal();
    }
    DLOG(INFO) << "proposal " << proposal->GetID() << " was promised by " << proposal->GetPercentagePromises() << "% of peers.";

    if(!proposal->StopTimer()){
      DLOG(ERROR) << "cancelling proposal " << proposal->GetID() << ", cannot stop timer.";
      return RejectProposal();
    }

    proposal->ResetCurrentVotes();
    return true;
  }

  bool ProposalJob::ExecutePhase2(){
    ProposalPtr proposal = GetProposal();
    DLOG(INFO) << "executing phase 2 for proposal " << proposal->raw();
    PeerSessionManager::BroadcastCommit();
    if(!proposal->StartTimer()){
      DLOG(ERROR) << "cancelling proposal " << proposal->raw() << ", cannot start timer.";
      return RejectProposal();
    }

    DLOG(INFO) << "waiting for " << proposal->GetRequired() << " votes....";
    while(proposal->GetCurrentVotes() < proposal->GetRequired());//spin

    if(proposal->GetPercentageRejects() > proposal->GetPercentageAccepts()){
      DLOG(WARNING) << "proposal " << proposal->GetID() << " was rejected by " << proposal->GetPercentageRejects() << "% of peers.";
      return RejectProposal();
    }
    DLOG(INFO) << "proposal " << proposal->GetID() << " was accepted by " << proposal->GetPercentageAccepts() << "% of peers.";

    if(!proposal->StopTimer()){
      DLOG(ERROR) << "cancelling proposal " << proposal->GetID() << ", cannot stop timer.";
      return RejectProposal();
    }

    proposal->ResetCurrentVotes();
    return true;
  }

  JobResult ProposalJob::DoWork(){
    if(!PauseMiner())
      return Failed("Cannot pause the block miner.");

    if(!ExecutePhase1())
      return Failed("Cannot execute phase 1");

    if(!ExecutePhase2())
      return Failed("Cannot execute phase 2");

    ProposalPtr proposal = GetProposal();
    if(proposal->GetTotalPercentageRejected() > proposal->GetTotalPercentageAccepted()){
      if(!RejectProposal())
        return Failed("Cannot broadcast Rejected");

      if(!SendRejectedToMiner())
        return Failed("Cannot send rejected to miner");

      std::stringstream ss;
      ss << "proposal " << proposal->GetID() << " was rejected (" << proposal->GetTotalPercentageRejected() << "%)";
      return Failed(ss);
    }

    if(!AcceptProposal())
      return Failed("Cannot broadcast Accepted");

    if(!CommitProposal())
      return Failed("Cannot commit the proposal");

    if(!SendAcceptedToMiner())
      return Failed("Cannot send accepted to miner");

    std::stringstream ss;
    ss << "proposal " << proposal->GetID() << " was accepted (" << proposal->GetTotalPercentageAccepted() << "%)";
    return Success(ss);
  }
}