#include <glog/logging.h>
#include <algorithm>
#include <mutex>
#include <condition_variable>

#include "proposal.h"
#include "message.h"

#include "block_miner.h"
#include "block_chain.h"
#include "block_pool.h"
#include "block_validator.h"
#include "block_handler.h"

namespace Token{
    static pthread_t thread_;
    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static uv_async_t aterm_;
    static uv_timer_t mine_timer_;
    static uv_timer_t proposal_timeout_timer_;
    static BlockMiner::State state_ = BlockMiner::State::kStopped;
    static Proposal* proposal_ = nullptr;

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

    void BlockMiner::SetProposal(Proposal* proposal){
        LOCK_GUARD;
        proposal_ = proposal;
    }

    bool BlockMiner::HasProposal(){
        LOCK_GUARD;
        return proposal_ != nullptr;
    }

    Proposal* BlockMiner::GetProposal(){
        LOCK_GUARD;
        return proposal_;
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

    void BlockMiner::Pause(){
#ifdef TOKEN_DEBUG
        LOG(INFO) << "pausing miner thread...";
#endif//TOKEN_DEBUG
        SetState(State::kPaused);
        uv_timer_stop(&mine_timer_);
    }

    void BlockMiner::Resume(){
#ifdef TOKEN_DEBUG
        LOG(INFO) << "resuming miner thread...";
#endif//TOKEN_DEBUG
        SetState(State::kRunning);
        uv_timer_again(&mine_timer_);
    }

    void BlockMiner::Shutdown(){
        if(!IsRunning()) return;
        uv_async_send(&aterm_);

        int ret;
        if((ret = pthread_join(thread_, NULL)) != 0){
            std::stringstream ss;
            ss << "Couldn't join block miner thread: " << strerror(ret);
            CrashReport::GenerateAndExit(ss);
        }
    }

    void* BlockMiner::MinerThread(void* data){
        SetState(State::kStarting);
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &aterm_, &HandleTerminateCallback);

        uv_timer_init(loop, &mine_timer_);
        uv_timer_init(loop, &proposal_timeout_timer_);

        uv_timer_start(&mine_timer_, &HandleMineBlockCallback, 0, BlockMiner::kMiningIntervalMilliseconds);

        SetState(State::kRunning);
        uv_run(loop, UV_RUN_DEFAULT);

        SetState(State::kStopped);
        uv_loop_close(loop);
        pthread_exit(0);
    }

    void BlockMiner::HandleTerminateCallback(uv_async_t* handle){
        uv_stop(handle->loop);
    }

    void BlockMiner::HandleProposalTimeoutCallback(uv_timer_t* handle){
        //TODO: implement
    }

    static inline bool
    MineBlock(Block* block, bool clean){
        BlockHeader header = block->GetHeader();
        if(!BlockHandler::ProcessBlock(block, clean)){
            LOG(WARNING) << "couldn't process block: " << header;
            return false;
        }
        BlockPool::RemoveBlock(header.GetHash());
        BlockChain::Append(block);
        return true;
    }

    void BlockMiner::HandleMineBlockCallback(uv_timer_t* handle){
        uint32_t num_transactions;
        if((num_transactions = TransactionPool::GetNumberOfTransactions()) >= kNumberOfTransactionsPerBlock){
            // 1. Collect transactions from pool
            Handle<Array<Transaction>> txs = Array<Transaction>::New(num_transactions);
            {
                // 1.a. Get list of transactions
                std::vector<uint256_t> pool_txs;
                if(!TransactionPool::GetTransactions(pool_txs)) CrashReport::GenerateAndExit("Couldn't get list of transactions in pool");

                // 1.b. Get data for list of transactions
                uint32_t index = 0;
                for(auto& it : pool_txs){
                    txs->Put(index++, TransactionPool::GetTransaction(it));
                }
            }

            // 2. Create new block
            Handle<Block> block = Block::NewInstance(BlockChain::GetHead(), txs, num_transactions);

            // 3. Validate block
            BlockValidator validator(block);
            if(!validator.IsValid()){
                LOG(WARNING) << "blocks isn't valid, orphaning!";
                return;
            }

            BlockPool::PutBlock(block);
            if(!HasProposal()){
                LOG(INFO) << Server::GetInfo().GetNodeID() << " creating proposal for: " << block->GetHeader();

                // 4. Create proposal
#ifdef TOKEN_DEBUG
                LOG(INFO) << "entering proposal phase";
#endif//TOKEN_DEBUG
                Handle<Proposal> proposal = Proposal::NewInstance(block, Server::GetInfo());
                SetProposal(proposal);

                // 5. Submit proposal
#ifdef TOKEN_DEBUG
                LOG(INFO) << "entering voting phase";
#endif//TOKEN_DEBUG
                proposal->SetPhase(Proposal::kVotingPhase);
                Handle<PrepareMessage> prepare_msg = PrepareMessage::NewInstance(proposal);
                Server::Broadcast(prepare_msg);
                proposal->WaitForRequiredVotes();

                // 6. Commit Proposal
#ifdef TOKEN_DEBUG
                LOG(INFO) << "entering commit phase";
#endif//TOKEN_DEBUG
                proposal->SetPhase(Proposal::kCommitPhase);
                Handle<CommitMessage> commit_msg = CommitMessage::NewInstance(proposal);
                Server::Broadcast(commit_msg);
                proposal->WaitForRequiredCommits();

                // 7. Finish Mining Block
                if(!MineBlock(block, true)){
                    //TODO: Handle better?
                    LOG(WARNING) << "couldn't mine block: " << block->GetHeader();
                }

                // 8. Quorum has been reached
                proposal->SetPhase(Proposal::kQuorumPhase);
            } else{
                // 4. Wait for Proposal to finish
                Proposal* proposal = GetProposal();
#ifdef TOKEN_DEBUG
                LOG(INFO) << "miner waiting for quorum on proposal....";
#endif//TOKEN_DEBUG
                proposal->WaitForQuorum();
#ifdef TOKEN_DEBUG
                LOG(INFO) << "proposal has finished, resuming mining...";
#endif//TOKEN_DEBUG
            }

            SetProposal(nullptr);
        }
    }
}