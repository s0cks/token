#include "consensus/proposal.h"

#include "miner.h"

#include "peer/peer_session_manager.h"

namespace token{
  const char* ProposalJob::kName = "ProposalJob";

  bool ProposalJob::ExecutePhase1(){
    Phase1Quorum& quorum = proposal_->GetPhase1Quorum();
    DLOG_JOB(INFO, this) << "executing proposal phase 1....";
    if(!proposal_->TransitionToPhase(ProposalPhase::kPreparePhase))
      return CancelProposal();
    quorum.StartTimer();

    // broadcast prepare to quorum members
    PeerSessionManager::BroadcastPrepare();

    // wait for required responses
    quorum.WaitForRequiredVotes();
    quorum.StopTimer();

    // was the proposal promised by the peers?
    QuorumResult result = quorum.GetResult();
    DLOG_JOB(INFO, this) << "phase 1 result: " << result;
    return true;
  }

  bool ProposalJob::ExecutePhase2() {
    NOT_IMPLEMENTED(WARNING);
    return false;
  }

  bool ProposalJob::CancelProposal(){
    DLOG_JOB(INFO, this) << "cancelling proposal....";
    NOT_IMPLEMENTED(WARNING);
    return false;
  }

  JobResult ProposalJob::DoWork(){
    if(!GetMiner()->Pause())
      return Failed("failed to pause the block miner.");

    if(!ExecutePhase1())
      return Failed("failed to execute phase 1.");

    if(!ExecutePhase2())
      return Failed("failed to execute phase 2.");

    if(!GetMiner()->Resume())
      return Failed("failed to resume the block miner.");

    return Success("proposal has finished.");
  }
}