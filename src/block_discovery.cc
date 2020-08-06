#include "block_discovery.h"

#include "block_pool.h"
#include "block_chain.h"
#include "block_queue.h"
#include "block_validator.h"
#include "transaction_pool.h"
#include "transaction_validator.h"

namespace Token{
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

        while(true){
            //TODO: be more stateful, prevent GC of live objects
            if(GetNumberOfTransactionsInPool() >= 2){
                Handle<Block> block = TransactionPoolBlockBuilder::Build();

                LOG(INFO) << "discovered block " << block << ", scheduling proposal....";
                BlockQueue::Queue(block);
            }
        }
    }
}