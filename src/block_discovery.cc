#include "block_discovery.h"

#include "block_pool.h"
#include "block_chain.h"
#include "block_queue.h"
#include "block_validator.h"
#include "transaction_pool.h"
#include "transaction_validator.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static std::condition_variable_any cond_;
    static Thread::State state_ = Thread::State::kStopped;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

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
            TransactionPool::RemoveTransaction(tx->GetSHA256Hash());
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
                Handle<Block> block = TransactionPoolBlockBuilder::Build();

                LOG(INFO) << "discovered block " << block << ", scheduling proposal....";
                BlockQueue::Queue(block); //TODO: pre-validate block before queuing?
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