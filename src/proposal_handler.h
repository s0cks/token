#ifndef TOKEN_PROPOSAL_HANDLER_H
#define TOKEN_PROPOSAL_HANDLER_H

#include <memory>
#include "proposal.h"
#include "async_task.h"
#include "block_verifier.h"
#include "block_processor.h"
#include "peer/peer_session_manager.h"

namespace Token{
    class ProposalHandler{
    protected:
        ProposalPtr proposal_;

        ProposalHandler(const ProposalPtr& proposal):
            proposal_(proposal){}

        bool WasRejected() const;
        bool CommitProposal() const;
        bool CancelProposal() const;
        bool TransitionToPhase(const Proposal::Phase& phase) const;

        static inline int32_t
        GetRequiredNumberOfPeers(){
            int32_t peers = PeerSessionManager::GetNumberOfConnectedPeers();
            if(peers == 0) return 0;
            else if(peers == 1) return 1;
            return peers / 2;
        }
    public:
        virtual ~ProposalHandler() = default;

        ProposalPtr GetProposal() const{
            return proposal_;
        }

        int64_t GetProposalID() const{
            return proposal_->GetHeight();
        }

        Hash GetProposalHash() const{
            return proposal_->GetHash();
        }

        std::shared_ptr<PeerSession> GetProposer() const{
            return proposal_->GetPeer();
        }

        Proposal::Result GetResult() const{
            return proposal_->GetResult();
        }

        virtual bool ProcessBlock(const BlockPtr& blk) const = 0;
        virtual bool ProcessProposal() const = 0;
    };

    class NewProposalHandler : public ProposalHandler{
    public:
        NewProposalHandler(const ProposalPtr& proposal):
            ProposalHandler(proposal){}
        ~NewProposalHandler() = default;

        bool ProcessBlock(const BlockPtr& blk) const{
            DefaultBlockProcessor processor;
            return blk->Accept(&processor);
        }

        bool ProcessProposal() const{
            // 1. Voting Phase
            if(!TransitionToPhase(Proposal::kVotingPhase))
                return false;
            PeerSessionManager::BroadcastPrepare();
            GetProposal()->WaitForRequiredResponses(); // TODO: add timeout here
            if(WasRejected())
                CancelProposal();

            // 2. Commit Phase
            if(!TransitionToPhase(Proposal::kCommitPhase))
                return false;
            PeerSessionManager::BroadcastCommit();
            GetProposal()->WaitForRequiredResponses(); //TODO: add timeout here
            if(WasRejected())
                CancelProposal();

            // 3. Quorum Phase
            CommitProposal();
            BlockDiscoveryThread::ClearProposal();
            return true;
        }
    };

    class PeerProposalHandler : public ProposalHandler{
    public:
        PeerProposalHandler(const ProposalPtr& proposal):
            ProposalHandler(proposal){}
        ~PeerProposalHandler() = default;

        bool ProcessBlock(const BlockPtr& blk) const{
            SynchronizeBlockProcessor processor;
            return blk->Accept(&processor);
        }

        bool ProcessProposal() const{
            std::shared_ptr<PeerSession> proposer = GetProposer();
            Hash hash = GetProposalHash();
            if(!BlockPool::HasBlock(hash)){
                //TODO: fix requesting of data
                LOG(INFO) << hash << " cannot be found, requesting block from peer: " << proposer->GetID();
                std::vector<InventoryItem> items = {
                    InventoryItem(InventoryItem::kBlock, hash)
                };
                GetProposer()->Send(GetDataMessage::NewInstance(items));
                LOG(INFO) << "waiting....";
                BlockPool::WaitForBlock(hash); //TODO: add timeout
                if(!BlockPool::HasBlock(hash)){
                    LOG(INFO) << "cannot resolve block " << hash << ", rejecting....";
                    return CancelProposal();
                }
            }

            BlockPtr blk = BlockPool::GetBlock(hash);
            LOG(INFO) << "proposal " << hash << " has entered the voting phase.";
            if(!BlockVerifier::IsValid(blk)){
                LOG(WARNING) << "cannot validate block " << hash << ", rejecting....";
                return CancelProposal();
            }
            proposer->SendAccepted();

            GetProposal()->WaitForPhase(Proposal::kCommitPhase);
            LOG(INFO) << "proposal " << hash << " has entered the commit phase.";
            if(!CommitProposal()){
                LOG(ERROR) << "couldn't commit proposal #" << GetProposalID() << ", rejecting....";
                return CancelProposal();
            }
            proposer->SendAccepted();

            GetProposal()->WaitForPhase(Proposal::kQuorumPhase);
            LOG(INFO) << "proposal " << hash << " has finished, result: " << GetResult();
            return true;
        }
    };
}

#endif //TOKEN_PROPOSAL_HANDLER_H