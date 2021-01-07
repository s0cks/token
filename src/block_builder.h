#ifndef TOKEN_BLOCK_BUILDER_H
#define TOKEN_BLOCK_BUILDER_H

#include <unordered_set>
#include "pool.h"
#include "blockchain.h"
#include "utils/printer.h"

namespace Token{
  class BlockBuilder : ObjectPoolTransactionVisitor{
   private:
    TransactionList transactions_;
   public:
    BlockBuilder():
      ObjectPoolTransactionVisitor(),
      transactions_(){}
    ~BlockBuilder() = default;

    BlockPtr Build(){
      if(!ObjectPool::VisitTransactions(this)){
        LOG(WARNING) << "couldn't visit object pool transactions.";
        return BlockPtr(nullptr);
      }

      std::sort(transactions_.begin(), transactions_.end(), Transaction::TimestampComparator());

      int64_t index = 0;
      TransactionSet transactions;
      for(auto& it : transactions_){
        TransactionPtr ntx = Transaction::NewInstance(index, it->inputs(), it->outputs(), it->GetTimestamp());
        if(!transactions.insert(ntx).second){
          LOG(WARNING) << "couldn't insert new transaction into transaction list for new block:";
          PrettyPrinter printer(google::WARNING, Printer::kFlagDetailed);
          printer(ntx);
          return BlockPtr(nullptr);
        }

        if((index++) >= Block::kMaxTransactionsForBlock){
          break;
        }
      }

      BlockPtr head = BlockChain::GetHead();
      LOG(INFO) << "creating block from parent: ";
      PrettyPrinter::PrettyPrint(head);

      BlockPtr blk = Block::NewInstance(head, transactions);
      Hash hash = blk->GetHash();
      if(!ObjectPool::PutBlock(hash, blk)){
        LOG(WARNING) << "couldn't put new block " << hash << " into the object pool.";
        return BlockPtr(nullptr);
      }
      return blk;
    }

    bool Visit(const TransactionPtr& tx){
      #ifdef TOKEN_DEBUG
      LOG(INFO) << "visiting transaction " << tx->GetHash() << "....";
      #endif//TOKEN_DEBUG
      transactions_.push_back(tx);
      return true;
    }

    static BlockPtr BuildNewBlock(){
      BlockBuilder builder;
      return builder.Build();
    }
  };
}

#endif //TOKEN_BLOCK_BUILDER_H