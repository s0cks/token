#include "pool.h"
#include "proposal_handler.h"
#include "snapshot/snapshot.h"

namespace token{
  bool ProposalHandler::CommitProposal() const{
    if(!TransitionToPhase(Proposal::kQuorumPhase))
      return false;

    Hash hash = GetProposal()->GetHash();
    BlockPtr blk = ObjectPool::GetBlock(hash);

    if(!ProcessBlock(blk)){
      LOG(WARNING) << "couldn't process block " << hash;
      return false;
    }

    if(!BlockChain::Append(blk)){
      LOG(WARNING) << "couldn't append block " << hash << ".";
      return false;
    }

    if(!ObjectPool::RemoveBlock(hash)){
      LOG(WARNING) << "couldn't remove block " << hash << " from pool.";
      return false;
    }

    if(FLAGS_enable_snapshots){
      LOG(INFO) << "scheduling new snapshot....";
      SnapshotJob* job = new SnapshotJob();
      if(!JobScheduler::Schedule(job))
        LOG(WARNING) << "couldn't schedule new snapshot.";
    }

    LOG(INFO) << "proposal " << proposal_->ToString() << " has finished.";
    return true;
  }

  bool ProposalHandler::CancelProposal() const{
    if(!TransitionToPhase(Proposal::kQuorumPhase))
      return false;
    GetProposal()->SetResult(Proposal::kRejected);
    return ProposalManager::ClearProposal();
  }

  bool ProposalHandler::WasRejected() const{
    return Proposal::GetRequiredNumberOfPeers() > 0
           && proposal_->GetNumberOfRejected() >= proposal_->GetNumberOfAccepted();
  }

#define CANNOT_TRANSITION_TO(From, To) \
    LOG(ERROR) << "cannot transition proposal #" << GetProposalID() << " from " << (From) << " phase to " << (To) << " phase.";

  bool ProposalHandler::TransitionToPhase(const Proposal::Phase& phase) const{
    return proposal_->TransitionToPhase(phase);
  }
}