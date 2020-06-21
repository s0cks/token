#include <glog/logging.h>
#include <algorithm>
#include <memory>

#include "node/node.h"

#include "block_miner.h"
#include "block_chain.h"
#include "block_validator.h"
#include "block_handler.h"

#include "paxos.h"

namespace Token{
    static pthread_t thread;

    static pthread_mutex_t proposal_mutex_ = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t proposal_cond_ = PTHREAD_COND_INITIALIZER;
    static Proposal* proposal_ = nullptr;

    static uv_timer_t mine_timer_;
    static uv_async_t exit_handle;

    Proposal* BlockMiner::GetProposal(){
        pthread_mutex_lock(&proposal_mutex_);
        Proposal* proposal = proposal_;
        pthread_mutex_unlock(&proposal_mutex_);
        return proposal;
    }

    void BlockMiner::SetProposal(Proposal* proposal){
        pthread_mutex_lock(&proposal_mutex_);
        proposal_ = proposal;
        pthread_mutex_unlock(&proposal_mutex_);
    }

    bool BlockMiner::HasProposal(){
        pthread_mutex_lock(&proposal_mutex_);
        bool found = proposal_ != nullptr;
        pthread_mutex_unlock(&proposal_mutex_);
        return found;
    }

    bool BlockMiner::SubmitProposal(Proposal* proposal){
        std::string node_id = Node::GetNodeID();
        LOG(INFO) << node_id << " creating proposal for: " << proposal->GetHeight() << "....";
        SetProposal(proposal);
        Node::Broadcast(PrepareMessage::NewInstance(node_id, (*proposal)));

        uint32_t votes_needed = Node::GetNumberOfPeers() > 0 ?
                                (Node::GetNumberOfPeers() == 1 ? 1 : (Node::GetNumberOfPeers() / 2)) :
                                0;

        pthread_mutex_lock(&proposal_mutex_);
        while(proposal->GetNumberOfVotes() < votes_needed){
            pthread_cond_wait(&proposal_cond_, &proposal_mutex_);
        }
        pthread_mutex_unlock(&proposal_mutex_);
        return true;
    }

    bool BlockMiner::CommitProposal(Proposal* proposal){
        std::string node_id = Node::GetNodeID();
        LOG(INFO) << node_id << " committing proposal for: " << proposal->GetHeight() << "....";
        Node::Broadcast(CommitMessage::NewInstance(node_id, (*proposal)));
        uint32_t commits_needed = Node::GetNumberOfPeers() > 0 ?
                                  (Node::GetNumberOfPeers() == 1 ? 1 : (Node::GetNumberOfPeers() / 2)) :
                                  0;

        pthread_mutex_trylock(&proposal_mutex_);
        while(proposal->GetNumberOfCommits() < commits_needed){
            pthread_cond_wait(&proposal_cond_, &proposal_mutex_);
        }
        pthread_mutex_unlock(&proposal_mutex_);
        return true;
    }

    bool BlockMiner::VoteForProposal(const std::string& node_id){
        Proposal* proposal;
        if(!(proposal = GetProposal())){
            LOG(WARNING) << "couldn't get the proposal";
            return false;
        }

        pthread_mutex_lock(&proposal_mutex_);
        bool voted = proposal->Vote(node_id);
        pthread_cond_signal(&proposal_cond_);
        pthread_mutex_unlock(&proposal_mutex_);
        return voted;
    }

    bool BlockMiner::AcceptProposal(const std::string& node_id){
        Proposal* proposal;
        if(!(proposal = GetProposal())){
            LOG(WARNING) << "couldn't get the proposal";
            return false;
        }

        pthread_mutex_trylock(&proposal_mutex_);
        bool accepted = proposal->Commit(node_id);
        pthread_cond_signal(&proposal_cond_);
        pthread_mutex_unlock(&proposal_mutex_);
        return accepted;
    }

    void BlockMiner::HandleMineCallback(uv_timer_t* handle){
        if(!HasProposal() && TransactionPool::GetSize() >= 2){
            // collect + sort transactions
            std::vector<uint256_t> all_txs;
            if(!TransactionPool::GetTransactions(all_txs)){
                LOG(ERROR) << "couldn't get transactions from transaction pool";
                pthread_mutex_unlock(&proposal_mutex_);
                return;
            }

            std::vector<Transaction*> txs;
            for(auto& it : all_txs){
                //TODO: fixme, terrible design?
                Transaction* tx;
                if(!(tx = TransactionPool::GetTransaction(it))){
                    LOG(WARNING) << "couldn't get transaction: " << it;
                    pthread_mutex_unlock(&proposal_mutex_);
                    return;
                }
                txs.push_back(tx);
            }

            // Create new block for proposal
            BlockHeader head = BlockChain::GetHead();
            LOG(INFO) << "<HEAD> := " << head;

            Block* block = Block::NewInstance(head, txs);
            if(!BlockPool::AddBlock(block)){
                LOG(WARNING) << "couldn't put newly mined block into block pool!";
                return;
            }

            BlockValidator validator(block);
            if(!validator.IsValid()){
                LOG(WARNING) << "blocks isn't valid, orphaning!";
                return;
            }

            uint256_t hash = block->GetHash();
            Proposal* proposal = new Proposal(Node::GetNodeID(), block);
            SetProposal(proposal);

            if(!SubmitProposal(proposal)){
                LOG(ERROR) << "couldn't submit proposal for new block: " << hash;
                goto cleanup;
            }

            if(!CommitProposal(proposal)){
                LOG(ERROR) << "couldn't commit proposal for new block: " << hash;
                goto cleanup;
            }

            if(!MineBlock(hash, block, true)){
                LOG(WARNING) << "couldn't mine block: " << hash;
                goto cleanup;
            }
        cleanup:
            SetProposal(nullptr);
            return;
        }
    }

    void BlockMiner::HandleExitCallback(uv_async_t* handle){
        if(uv_is_closing((uv_handle_t*)handle) == 0){
            uv_close((uv_handle_t*)handle, NULL);
        }
        pthread_exit(nullptr);
    }

    void* BlockMiner::MinerThread(void* data){
        pthread_mutex_init(&proposal_mutex_, NULL);
        pthread_cond_init(&proposal_cond_, NULL);
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &exit_handle, &HandleExitCallback);
        uv_timer_init(loop, &mine_timer_);
        uv_timer_start(&mine_timer_, &HandleMineCallback, 0, BlockMiner::kMiningIntervalMilliseconds);
        uv_run(loop, UV_RUN_DEFAULT);
        pthread_exit(nullptr);
    }

    bool BlockMiner::Initialize(){
        return pthread_create(&thread, NULL, &BlockMiner::MinerThread, nullptr) == 0;
    }

    bool BlockMiner::MineBlock(const uint256_t& hash, Block* block, bool clean){
        if(!BlockHandler::ProcessBlock(block, clean)){
            LOG(WARNING) << "couldn't process block: " << hash;
            return false;
        }
        if(!BlockPool::RemoveBlock(hash)){
            LOG(ERROR) << "couldn't remove new block from pool: " << hash;
            return false;
        }
        if(!BlockChain::GetInstance()->Append(block)){
            LOG(ERROR) << "couldn't append new block: " << hash;
            return false;
        }
        return true;
    }
}