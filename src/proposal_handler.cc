#include "block_discovery.h"
#include "proposal_handler.h"

namespace Token{
    bool ProposalHandler::CommitProposal() const{
        Hash hash = GetProposal()->GetHash();
        BlockPtr blk = BlockPool::GetBlock(hash);
        if(!ProcessBlock(blk)){
            LOG(WARNING) << "couldn't process block " << hash << ".";
            return false;
        }

        if(!BlockChain::Append(blk)){
            LOG(WARNING) << "couldn't append block " << hash << ".";
            return false;
        }

        if(!BlockPool::RemoveBlock(hash)){
            LOG(WARNING) << "couldn't remove block " << hash << " from pool.";
            return false;
        }
#ifdef TOKEN_DEBUG
        //TODO: convert to SnapshotManager class?
        if(FLAGS_enable_snapshots){
            LOG(INFO) << "scheduling new snapshot....";
            SnapshotTask* task = SnapshotTask::NewInstance();
            if(!task->Submit())
                LOG(WARNING) << "couldn't schedule new snapshot!";
        }
#endif//TOKEN_DEBUG
        return true;
    }

    bool ProposalHandler::CancelProposal() const{
        if(!TransitionToPhase(Proposal::kQuorumPhase))
            return false;
        GetProposal()->SetResult(Proposal::kRejected);
        return true;
    }

    bool ProposalHandler::WasRejected() const{
        return GetRequiredNumberOfPeers() > 0
            && proposal_->GetNumberOfRejected() >= proposal_->GetNumberOfAccepted();
    }

#define CANNOT_TRANSITION_TO(From, To) \
    LOG(ERROR) << "cannot transition proposal #" << GetProposalID() << " from " << (From) << " phase to " << (To) << " phase.";

    bool ProposalHandler::TransitionToPhase(const Proposal::Phase& phase) const{
        //TODO: better error handling
        LOG(INFO) << "transitioning proposal #" << GetProposalID() << " to phase: " << phase;
        switch(phase){
            case Proposal::kVotingPhase:
                if(!GetProposal()->IsProposal()){
                    CANNOT_TRANSITION_TO(GetProposal()->GetPhase(), phase);
                    return false;
                }
                GetProposal()->SetPhase(phase);
                return true;
            case Proposal::kCommitPhase:
                if(!GetProposal()->IsVoting()){
                    CANNOT_TRANSITION_TO(GetProposal()->GetPhase(), phase);
                    return false;
                }
                GetProposal()->SetPhase(phase);
                return true;
            case Proposal::kQuorumPhase:
                if(!(GetProposal()->IsVoting() || GetProposal()->IsCommit())){
                    CANNOT_TRANSITION_TO(GetProposal()->GetPhase(), phase);
                    return false;
                }
                GetProposal()->SetPhase(phase);
                return true;
            case Proposal::kProposalPhase:
            default:
                CANNOT_TRANSITION_TO(GetProposal()->GetPhase(), phase);
                return false;
        }
    }
}