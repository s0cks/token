#include "sync_block_committer.h"
#include "sync_transaction_committer.h"

namespace token{
  namespace sync{
    bool BlockCommitter::Visit(const IndexedTransactionPtr& tx){
      auto hash = tx->hash();
      DVLOG(2) << "visiting transaction " << hash << "....";
      sync::TransactionCommitter committer(&batch_, hash, tx);
      if(!tx->VisitInputs(&committer)){
        LOG(ERROR) << "cannot visit transaction inputs.";
        return false;
      }
      if(!tx->VisitOutputs(&committer)){
        LOG(ERROR) << "cannot visit transaction outputs.";
        return false;
      }
      return true;
    }

    bool BlockCommitter::Commit(const Hash& hash){
      DLOG(INFO) << "committing block " << hash << "....";
#ifdef TOKEN_DEBUG
      auto start = Clock::now();
#endif//TOKEN_DEBUG

      auto blk = Block::Genesis();//TODO: get block from pool
      if(!blk->VisitTransactions(this)){
        LOG(ERROR) << "cannot visit block transactions.";
        return false;
      }

      leveldb::Status status;
      if(!(status = GetObjectPool().Commit(batch_.batch())).ok()){
        LOG(ERROR) << "cannot commit batch to object pool: " << status.ToString();
        return false;
      }

#ifdef TOKEN_DEBUG
      DLOG(INFO) << hash << " sync commit has finished, took " << GetElapsedTimeMilliseconds(start) << "ms.";
#endif//TOKEN_DEBUG
      return true;
    }
  }
}