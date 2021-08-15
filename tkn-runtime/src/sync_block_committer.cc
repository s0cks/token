#include "block.h"
#include "sync_block_committer.h"
#include "sync_transaction_committer.h"

namespace token{
  namespace sync{
    bool BlockCommitter::Visit(const IndexedTransactionPtr& tx){
      auto hash = tx->hash();
      DVLOG(2) << "visiting transaction " << hash << "....";
      sync::TransactionCommitter committer(nullptr, hash, tx);
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

    bool BlockCommitter::Commit(){
      DLOG(INFO) << "committing block " << GetBlockHash() << "....";
#ifdef TOKEN_DEBUG
      auto start = Clock::now();
#endif//TOKEN_DEBUG

      if(!GetBlock()->VisitTransactions(this)){
        LOG(ERROR) << "cannot visit block transactions.";
        return false;
      }

      if(!utxos().Commit(&batch_)){
        LOG(ERROR) << "cannot commit batch to pool.";
        return false;
      }

#ifdef TOKEN_DEBUG
      DLOG(INFO) << GetBlockHash() << " sync commit has finished, took " << GetElapsedTimeMilliseconds(start) << "ms.";
#endif//TOKEN_DEBUG
      return true;
    }
  }
}