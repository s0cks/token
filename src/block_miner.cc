#include <glog/logging.h>
#include <algorithm>
#include <memory>

#include "node/node.h"

#include "block_miner.h"
#include "block_chain.h"
#include "block_validator.h"
#include "block_handler.h"

#include "proposal.h"

namespace Token{
    static pthread_t thread;
    static uv_loop_t* loop_ = nullptr;

    static pthread_mutex_t proposal_mutex_ = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t proposal_cond_ = PTHREAD_COND_INITIALIZER;
    static Proposal* proposal_ = nullptr;

    static BlockMiner::State state_ = BlockMiner::State::kStopped;
    static pthread_mutex_t state_mutex_ = PTHREAD_MUTEX_INITIALIZER;
    static pthread_cond_t state_cond_ = PTHREAD_COND_INITIALIZER;

    static uv_timer_t mine_timer_;
    static uv_async_t exit_handle;

    void BlockMiner::SetState(BlockMiner::State state){
        pthread_mutex_trylock(&state_mutex_);
        state_ = state;
        pthread_cond_signal(&state_cond_);
        pthread_mutex_unlock(&state_mutex_);
    }

    BlockMiner::State BlockMiner::GetState(){
        pthread_mutex_trylock(&state_mutex_);
        BlockMiner::State state = state_;
        pthread_mutex_unlock(&state_mutex_);
        return state;
    }

    void BlockMiner::WaitForState(State state){
        pthread_mutex_trylock(&state_mutex_);
        while(state_ != state) pthread_cond_wait(&state_cond_, &state_mutex_);
        pthread_mutex_unlock(&state_mutex_);
    }

    void BlockMiner::WaitForShutdown(){
        WaitForState(State::kStopped);
    }

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

    void BlockMiner::HandleTerminateCallback(uv_async_t* handle){
        //TODO: implement
    }

    //TODO: check state
    void BlockMiner::HandleMineCallback(uv_timer_t* handle){
        if(!HasProposal() && TransactionPool::GetSize() >= 2){
            LOG(INFO) << "creating proposal...";

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
            BlockPool::PutBlock(block);

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

    void* BlockMiner::MinerThread(void* data){
        SetState(State::kRunning);
        uv_loop_t* loop = loop_ = uv_loop_new();
        uv_async_init(loop, &exit_handle, &HandleTerminateCallback);
        uv_timer_init(loop, &mine_timer_);
        uv_timer_start(&mine_timer_, &HandleMineCallback, 0, BlockMiner::kMiningIntervalMilliseconds);
        uv_run(loop, UV_RUN_DEFAULT);
        pthread_exit(0);
    }

    bool BlockMiner::Initialize(){
        if(!IsStopped()) return false;
        return pthread_create(&thread, NULL, &BlockMiner::MinerThread, nullptr) == 0;
    }

    bool BlockMiner::Shutdown(){
        if(!IsRunning()) return false;
        uv_async_send(&exit_handle);
        return pthread_join(thread, NULL) == 0;
    }

    bool BlockMiner::MineBlock(const uint256_t& hash, Block* block, bool clean){
        if(!BlockHandler::ProcessBlock(block, clean)){
            LOG(WARNING) << "couldn't process block: " << hash;
            return false;
        }
        BlockPool::RemoveBlock(hash);
        BlockChain::Append(block);
        return true;
    }
}