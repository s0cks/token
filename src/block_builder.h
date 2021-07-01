#ifndef TOKEN_BLOCK_BUILDER_H
#define TOKEN_BLOCK_BUILDER_H

#include <unordered_set>
#include "pool.h"
#include "blockchain.h"

namespace token{
  class BlockBuilder : ObjectPoolTransactionVisitor{
   private:
    BlockPtr parent_;
    TransactionList transactions_;
   public:
    BlockBuilder(const BlockChainPtr& chain):
      ObjectPoolTransactionVisitor(),
      parent_(chain->GetHead()),
      transactions_(){}
    ~BlockBuilder() = default;

    BlockPtr GetParent() const{
      return parent_;
    }

    //TODO: refactor
    BlockPtr Build(){
      ObjectPoolPtr pool = ObjectPool::GetInstance();

      if(!pool->VisitTransactions(this)){
        LOG(WARNING) << "couldn't visit object pool transactions.";
        return BlockPtr(nullptr);
      }

      std::sort(transactions_.begin(), transactions_.end(), Transaction::TimestampComparator());

      int64_t blk_size = 0;

      int64_t index = 0;
      IndexedTransactionSet transactions;
      for(auto& it : transactions_){
        IndexedTransactionPtr ntx = IndexedTransaction::NewInstance(index++, it->inputs(), it->outputs(), it->timestamp());
        if((blk_size + ntx->GetBufferSize()) >= Block::kMaxBlockSize)
          break;

        if(!transactions.insert(ntx).second){
          LOG(WARNING) << "couldn't insert new transaction into transaction list for new block:";
          return BlockPtr(nullptr);
        }
      }

      BlockPtr blk = Block::FromParent(GetParent(), transactions);
      Hash hash = blk->hash();
      if(!pool->PutBlock(hash, blk)){
        LOG(WARNING) << "couldn't put new block " << hash << " into the object pool.";
        return BlockPtr(nullptr);
      }
      return blk;
    }

    bool Visit(const TransactionPtr& tx){
      transactions_.push_back(tx);
      return true;
    }

    static BlockPtr BuildNewBlock(){
      BlockBuilder builder(BlockChain::GetInstance());
      return builder.Build();
    }
  };
}

#endif //TOKEN_BLOCK_BUILDER_H