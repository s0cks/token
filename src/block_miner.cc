#include <glog/logging.h>
#include <algorithm>
#include <mutex>
#include <condition_variable>

#include "alloc/scope.h"
#include "node/node.h"

#include "block_miner.h"
#include "block_chain.h"
#include "block_validator.h"
#include "block_handler.h"

#include "proposal.h"

namespace Token{
    static pthread_t thread_;
    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static BlockMiner::State state_ = BlockMiner::State::kStopped;
    static Proposal* proposal_ = nullptr;
    static uv_async_t aterm_;

    static uv_timer_t mine_timer_;


#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    void BlockMiner::SetState(BlockMiner::State state){
        LOCK;
        state_ = state;
        SIGNAL_ALL;
    }

    BlockMiner::State BlockMiner::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void BlockMiner::WaitForState(State state){
        LOCK;
        while(state_ != state) WAIT;
    }

    Proposal* BlockMiner::GetProposal(){
        LOCK_GUARD;
        return proposal_;
    }

    void BlockMiner::SetProposal(Proposal* proposal){
        LOCK_GUARD;
        proposal_ = proposal;
    }

    bool BlockMiner::HasProposal(){
        LOCK_GUARD;
        return proposal_ != nullptr;
    }

    bool BlockMiner::SubmitProposal(Proposal* proposal){
        std::string node_id = Node::GetNodeID();
        LOG(INFO) << node_id << " creating proposal for: " << proposal->GetHeight() << "....";
        SetProposal(proposal);
        Node::Broadcast(PrepareMessage::NewInstance(node_id, (*proposal)));

        uint32_t votes_needed = Node::GetNumberOfPeers() > 0 ?
                                (Node::GetNumberOfPeers() == 1 ? 1 : (Node::GetNumberOfPeers() / 2)) :
                                0;

        LOCK;
        while(proposal->GetNumberOfVotes() < votes_needed) WAIT;
        return true;
    }

    bool BlockMiner::CommitProposal(Proposal* proposal){
        std::string node_id = Node::GetNodeID();
        LOG(INFO) << node_id << " committing proposal for: " << proposal->GetHeight() << "....";
        Node::Broadcast(CommitMessage::NewInstance(node_id, (*proposal)));
        uint32_t commits_needed = Node::GetNumberOfPeers() > 0 ?
                                  (Node::GetNumberOfPeers() == 1 ? 1 : (Node::GetNumberOfPeers() / 2)) :
                                  0;

        LOCK;
        while(proposal->GetNumberOfCommits() < commits_needed) WAIT;
        return true;
    }

    bool BlockMiner::VoteForProposal(const std::string& node_id){
        Proposal* proposal;
        if(!(proposal = GetProposal())){
            LOG(WARNING) << "couldn't get the proposal";
            return false;
        }

        LOCK;
        bool voted = proposal->Vote(node_id);
        SIGNAL_ALL;
        return voted;
    }

    bool BlockMiner::AcceptProposal(const std::string& node_id){
        Proposal* proposal;
        if(!(proposal = GetProposal())){
            LOG(WARNING) << "couldn't get the proposal";
            return false;
        }

        LOCK;
        bool accepted = proposal->Commit(node_id);
        SIGNAL_ALL;
        return accepted;
    }

    void BlockMiner::HandleTerminateCallback(uv_async_t* handle){
        uv_stop(handle->loop);
    }

    void BlockMiner::HandleMineCallback(uv_timer_t* handle){
        Scope scope;
        if(TransactionPool::GetNumberOfTransactions() >= 2){
#ifdef TOKEN_DEBUG
            LOG(INFO) << "mining new block....";
#endif//TOKEN_DEBUG

            // 1. Collect transactions from pool
            Block::TransactionList all_txs;
            {
                // 1.a. Get list of transactions
                std::vector<uint256_t> pool_txs;
                if(!TransactionPool::GetTransactions(pool_txs)) CrashReport::GenerateAndExit("Couldn't get list of transactions in pool");

                // 1.b. Get data for list of transactions
                for(auto& it : pool_txs){
                    Transaction* tx = scope.Retain(TransactionPool::GetTransaction(it));
#ifdef TOKEN_DEBUG
                    LOG(INFO) << "using transaction: " << it;
#endif//TOKEN_DEBUG
                    all_txs.push_back(tx);
                }
            }

            // 2. Create new block
            Block* block = scope.Retain(Block::NewInstance(BlockChain::GetHead(), all_txs));

            // 3. Validate block
            BlockValidator validator(block);
            if(!validator.IsValid()){
                LOG(WARNING) << "blocks isn't valid, orphaning!";
                return;
            }

            BlockPool::PutBlock(block);
            if(!HasProposal()){
                // 4. Create proposal

                BlockHeader header = block->GetHeader();
                Proposal* proposal = new Proposal(Node::GetNodeID(), block);

                // 5. Submit proposal
                if(!SubmitProposal(proposal)){
                    LOG(ERROR) << "couldn't submit proposal for new block: " << header;
                    goto cleanup;
                }

                // 6. Commit proposal
                if(!CommitProposal(proposal)){
                    LOG(ERROR) << "couldn't commit proposal for new block: " << header;
                    goto cleanup;
                }

                // 7. Finish mining block
                if(!MineBlock(block, true)){
                    LOG(WARNING) << "couldn't mine block: " << header;
                    goto cleanup;
                }

                cleanup:
                SetProposal(nullptr);//TODO: memory leak
            } else{
                // 4. Wait for Proposal to finish
                Proposal* proposal = GetProposal();
            }
        }
    }

    void* BlockMiner::MinerThread(void* data){
        SetState(State::kStarting);
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &aterm_, &HandleTerminateCallback);

        uv_timer_init(loop, &mine_timer_);
        uv_timer_start(&mine_timer_, &HandleMineCallback, 0, BlockMiner::kMiningIntervalMilliseconds);

        SetState(State::kRunning);
        uv_run(loop, UV_RUN_DEFAULT);

        SetState(State::kStopped);
        uv_loop_close(loop);
        pthread_exit(0);
    }

    void BlockMiner::Initialize(){
        if(!IsStopped()) return;
        int ret;
        if((ret = pthread_create(&thread_, NULL, &MinerThread, NULL)) != 0){
            std::stringstream ss;
            ss << "Couldn't start the block miner thread: " << strerror(ret);
            CrashReport::GenerateAndExit(ss);
        }
    }

    bool BlockMiner::Shutdown(){
        if(!IsRunning()) return false;
        uv_async_send(&aterm_);

        int ret;
        if((ret = pthread_join(thread_, NULL)) != 0){
            std::stringstream ss;
            ss << "Couldn't join block miner thread: " << strerror(ret);
            CrashReport::GenerateAndExit(ss);
        }
        return true;
    }

    bool BlockMiner::MineBlock(Block* block, bool clean){
        BlockHeader header = block->GetHeader();
        if(!BlockHandler::ProcessBlock(block, clean)){
            LOG(WARNING) << "couldn't process block: " << header;
            return false;
        }
        BlockPool::RemoveBlock(header.GetHash());
        BlockChain::Append(block);
        return true;
    }
}