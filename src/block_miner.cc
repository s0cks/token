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

#include "async_task.h"

namespace Token{
    static pthread_t thread_;
    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static uv_async_t aterm_;
    static uv_timer_t mine_timer_;
    static uv_timer_t proposal_timeout_timer_;
    static BlockMiner::State state_ = BlockMiner::State::kStopped;

    static Handle<Proposal> proposal_ = Handle<Proposal>();

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

    void BlockMiner::SetProposal(const Handle<Proposal>& proposal){
        LOCK_GUARD;
        proposal_ = proposal;

        LOG(INFO) << "starting proposal timer for " << (kProposalTimeoutMilliseconds / 1000) << " seconds...";
        uv_timer_start(&proposal_timeout_timer_, &HandleProposalTimeoutCallback, kProposalTimeoutMilliseconds, 0);
    }

    bool BlockMiner::HasProposal(){
        LOCK_GUARD;
        return proposal_ != nullptr;
    }

    Handle<Proposal> BlockMiner::GetProposal(){
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
        Handle<Proposal> proposal = GetProposal();
        LOG(INFO) << "proposal #" << proposal->GetHeight() << " timed out!";
        proposal->SetPhase(Proposal::kQuorumPhase); // should we use kQuorum? maybe redefine the quorum phase and have a flag attached for status?
        SetProposal(nullptr);
    }

    static inline size_t
    GetNumberOfTransactionsInPool(){
        return TransactionPool::GetNumberOfTransactions();
    }

    class TransactionPoolBlockBuilder : public TransactionPoolVisitor{
    private:
        size_t num_transactions_;
        size_t index_;
        Transaction** transactions_;

        inline void
        AddTransaction(const Handle<Transaction>& tx){
            transactions_[index_++] = tx;
        }
    public:
        TransactionPoolBlockBuilder():
            TransactionPoolVisitor(),
            num_transactions_(GetNumberOfTransactionsInPool()),
            index_(0),
            transactions_(nullptr){
            if(num_transactions_ > 0){
                transactions_ = (Transaction**)malloc(sizeof(Transaction*)*num_transactions_);
                memset(transactions_, 0, sizeof(Transaction*)*num_transactions_);
            }
        }
        ~TransactionPoolBlockBuilder(){
            if(num_transactions_ > 0 && transactions_) free(transactions_);
        }

        Handle<Block> GetBlock() const{
            BlockHeader parent = BlockChain::GetHead();
            return Block::NewInstance(parent, transactions_, num_transactions_);
        }

        bool Visit(const Handle<Transaction>& tx){
            if(TransactionValidator::IsValid(tx)) AddTransaction(tx);
            return true;
        }

        static Handle<Block> Build(){
            TransactionPoolBlockBuilder builder;
            TransactionPool::Accept(&builder);
            Handle<Block> block = builder.GetBlock();
            BlockPool::PutBlock(block);
            return block;
        }
    };

    static inline bool
    MineBlock(const Handle<Block>& block, bool clean){
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
        if(!HasProposal()){
            if(GetNumberOfTransactionsInPool() < kNumberOfTransactionsPerBlock) return; // do nothing?....
            Handle<Block> block = TransactionPoolBlockBuilder::Build();

            // 3. Validate block
            BlockValidator validator(block);
            if(!validator.IsValid()){
                LOG(WARNING) << "blocks isn't valid, orphaning!";
                return;
            }

            LOG(INFO) << Server::GetID() << " creating proposal for: " << block->GetHeader();

            // 4. Create proposal
#ifdef TOKEN_DEBUG
            LOG(INFO) << "entering proposal phase";
#endif//TOKEN_DEBUG
            Handle<Proposal> proposal = Proposal::NewInstance(block, Server::GetID());
            SetProposal(proposal);

            Handle<HandleProposalTask> task = HandleProposalTask::NewInstance(handle->loop, proposal, block);
            if(!task->Submit()){
                LOG(WARNING) << "couldn't submit HandleProposal task for proposal: #" << proposal->GetHeight();
                return;
            }
        } else{
            // 4. Wait for Proposal to finish
            Handle<Proposal> proposal = GetProposal();
#ifdef TOKEN_DEBUG
            LOG(INFO) << "miner waiting for quorum on proposal....";
#endif//TOKEN_DEBUG

            proposal->WaitForQuorum();

            uint256_t hash = proposal->GetHash();
            if(!BlockPool::HasBlock(hash)){
                LOG(WARNING) << "block " << hash << " not found in pool, cannot finish proposal: #" << proposal->GetHeight();
                return;
            }

            Handle<Block> block = BlockPool::GetBlock(hash);

            // 7. Finish Mining Block
            if(!MineBlock(block, false)){ //TODO: yes we want to clean the transaction pool as well
                //TODO: Handle better?
                LOG(WARNING) << "couldn't mine block: " << block->GetHeader();
            }

#ifdef TOKEN_DEBUG
            LOG(INFO) << "proposal has finished, resuming mining...";
#endif//TOKEN_DEBUG
            SetProposal(nullptr);
        }
    }
}