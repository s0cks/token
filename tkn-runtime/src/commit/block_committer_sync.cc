#include "../../../Sources/token/block.h"
#include "block_committer_sync.h"

namespace token{
  namespace sync{
    bool TransactionCommitter::Visit(const InputPtr& val){
      //TODO: remove inputs

      NOT_IMPLEMENTED(ERROR);//TODO: implement
      return true;
    }

    bool TransactionCommitter::Visit(const OutputPtr& val){
      auto index = output_idx_++;
      DVLOG(2) << "visiting output #" << index << " " << val->ToString() << "....";
      auto utxo = std::make_shared<UnclaimedTransaction>();//TODO: fixme
      auto utxo_hash = utxo->hash();
      DVLOG(1) << "created unclaimed transaction " << utxo_hash << " from " << TransactionReference(hash(), index);
      batch()->Put(utxo_hash, utxo);
      return true;
    }

    bool BlockCommitter::Visit(const IndexedTransactionPtr& tx){
      auto hash = tx->hash();
      DVLOG(2) << "visiting transaction " << hash << "....";
      sync::TransactionCommitter committer(batch_, hash, tx);
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

      if(!block()->VisitTransactions(this)){
        LOG(ERROR) << "cannot visit block transactions.";
        return false;
      }

      if(!utxos().Commit(batch_.get())){
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