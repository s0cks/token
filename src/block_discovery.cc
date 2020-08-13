#include "block_discovery.h"
#include "block_pool.h"
#include "block_chain.h"
#include "block_queue.h"
#include "block_validator.h"
#include "block_handler.h"
#include "transaction_pool.h"
#include "transaction_validator.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static Thread::State state_ = Thread::State::kStopped;

    static Block* block_ = nullptr;
    static Proposal* proposal_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

#define CHECK_FOR_REJECTION(Proposal) \
    if((Proposal)->IsRejected()){ \
        OnRejected((Proposal)); \
    }
#define CHECK_STATUS(ProposalInstance) \
    if((ProposalInstance)->GetNumberOfRejected() > (ProposalInstance)->GetNumberOfAccepted()){ \
        (ProposalInstance)->SetStatus(Proposal::Status::kRejectedStatus); \
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

        inline void
        RemoveTransactionFromPool(const Handle<Transaction>& tx){
            TransactionPool::RemoveTransaction(tx->GetHash());
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
            if(TransactionValidator::IsValid(tx)){
                AddTransaction(tx);
                RemoveTransactionFromPool(tx);
            }
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

    static inline void
    OrphanTransaction(const Handle<Transaction>& tx){
        uint256_t hash = tx->GetHash();
        LOG(WARNING) << "orphaning transaction: " << hash;
        TransactionPool::RemoveTransaction(hash);
    }

    static inline void
    OrphanBlock(const Handle<Block>& blk){
        uint256_t hash = blk->GetHash();
        LOG(WARNING) << "orphaning block: " << hash;
        BlockPool::RemoveBlock(hash);
    }

    void BlockDiscoveryThread::HandleVotingPhase(const Handle<Proposal>& proposal){
        LOG(INFO) << "proposal " << proposal << " entering voting phase";
        proposal->SetPhase(Proposal::Phase::kVotingPhase);
        Server::Broadcast(PrepareMessage::NewInstance(proposal, Server::GetID()));
        proposal->WaitForRequiredResponses();
        CHECK_STATUS(proposal);
    }

    void BlockDiscoveryThread::HandleCommitPhase(const Handle<Proposal>& proposal){
        LOG(INFO) << "proposal " << proposal << " entering commit phase";
        proposal->SetPhase(Proposal::Phase::kCommitPhase);
        Server::Broadcast(CommitMessage::NewInstance(proposal, Server::GetID()));
        proposal->WaitForRequiredResponses();
        CHECK_STATUS(proposal);
    }

    void BlockDiscoveryThread::HandleQuorumPhase(const Handle<Proposal>& proposal){
        LOG(INFO) << "proposal " << proposal << " entering quorum phase";
        proposal->SetPhase(Proposal::Phase::kQuorumPhase);
        CHECK_FOR_REJECTION(proposal);
        LOG(INFO) << "proposal " << proposal << " was accepted";
        proposal->SetStatus(Proposal::Status::kAcceptedStatus);
        OnAccepted(proposal);
    }

    void BlockDiscoveryThread::OnAccepted(const Handle<Proposal>& proposal){
        Handle<Block> blk = GetBlock();
        if(!BlockHandler::ProcessBlock(blk, true)){
            LOG(WARNING) << "couldn't process block: " << blk;
            return;
        }
        BlockPool::RemoveBlock(proposal->GetHash());
        BlockChain::Append(blk);
        SetProposal(nullptr);
    }

    void BlockDiscoveryThread::OnRejected(const Handle<Proposal>& proposal){

    }

    Handle<Block> BlockDiscoveryThread::CreateNewBlock(){
        Handle<Block> blk = TransactionPoolBlockBuilder::Build();
        SetBlock(blk);
        return blk;
    }

    Handle<Proposal> BlockDiscoveryThread::CreateNewProposal(const Handle<Block>& blk){
        Handle<Proposal> proposal = Proposal::NewInstance(blk, Server::GetID());
        SetProposal(proposal);
        return proposal;
    }

    void BlockDiscoveryThread::SetBlock(const Handle<Block>& blk){
        LOCK_GUARD;
        block_ = blk;
    }

    void BlockDiscoveryThread::SetProposal(const Handle<Proposal>& proposal){
        LOCK_GUARD;
        proposal_ = proposal;
    }

    bool BlockDiscoveryThread::HasProposal(){
        LOCK_GUARD;
        return proposal_ != nullptr;
    }

    Handle<Block> BlockDiscoveryThread::GetBlock(){
        LOCK_GUARD;
        return block_;
    }

    Handle<Proposal> BlockDiscoveryThread::GetProposal(){
        LOCK_GUARD;
        return proposal_;
    }

    void BlockDiscoveryThread::HandleThread(uword parameter){
        LOG(INFO) << "starting the block discovery thread....";
        SetState(Thread::State::kRunning);
        while(!IsStopped()){
            if(IsPaused()){
                LOG(INFO) << "pausing block discovery thread....";

                LOCK;
                while(GetState() != Thread::State::kRunning || GetState() != Thread::State::kStopped) WAIT;
                UNLOCK;

                if(IsStopped()){
                    LOG(WARNING) << "block discovery thread has been halted, exiting.";
                    return;
                }

                LOG(INFO) << "block discovery thread is resuming.";
            }

            if(GetNumberOfTransactionsInPool() >= 2){
                Handle<Block> block = CreateNewBlock();

                BlockValidator validator(block);
                if(!validator.IsValid()){
                    LOG(WARNING) << "invalid block discovered: " << block;
                    LOG(WARNING) << "removing " << validator.GetNumberOfInvalidTransactions() << " invalid transactions from pool....";
                    auto iter = validator.invalid_begin();
                    while(iter != validator.invalid_end()){
                        Handle<Transaction> invalid_tx = (*iter);
                        OrphanTransaction(invalid_tx);
                        iter++;
                    }
                    OrphanBlock(block);
                    continue;
                }

                LOG(INFO) << "discovered block " << block << ", creating proposal....";
                Handle<Proposal> proposal = CreateNewProposal(block);

                HandleVotingPhase(proposal);
                CHECK_FOR_REJECTION(proposal);
                HandleCommitPhase(proposal);
                CHECK_FOR_REJECTION(proposal);
                HandleQuorumPhase(proposal);
            }
        }
    }

    Thread::State BlockDiscoveryThread::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void BlockDiscoveryThread::SetState(Thread::State state){
        LOCK_GUARD;
        state_ = state;
    }

    void BlockDiscoveryThread::WaitForState(Thread::State state){
        LOCK;
        while(GetState() != state) WAIT;
    }
}