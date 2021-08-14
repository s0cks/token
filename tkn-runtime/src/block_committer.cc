#include "block.h"
#include "timestamp.h"
#include "block_committer.h"

namespace token{
  bool SyncBlockCommitter::Visit(const Input& val){
    DVLOG(2) << "visiting "
  }

  bool SyncBlockCommitter::Visit(const Output& val){

  }

  bool SyncBlockCommitter::Visit(const IndexedTransactionPtr& tx){
    auto hash = tx->hash();
    DLOG(INFO) << "visiting transaction " << hash;
    if(!tx->VisitInputs(this)){
      DLOG(ERROR) << "cannot visit transaction " << hash << " inputs.";
      return false;
    }

    if(!tx->VisitOutputs(this)){
      DLOG(ERROR) << "cannot visit transaction " << hash << " outputs.";
      return false;
    }
    return true;
  }

  bool SyncBlockCommitter::Commit(const Hash& hash){
#ifdef TOKEN_DEBUG
    auto start = Clock::now();
#endif//TOKEN_DEBUG

    auto blk = Block::Genesis();//TODO: get block from pool
    if(!blk->VisitTransactions(this)){
      LOG(ERROR) << "cannot visit block transactions.";
      return false;
    }


//    for(auto& it : transactions){
//      std::vector<Output> outputs;
//      it->GetOutputs(outputs);
//
//      uint64_t index = 0;
//      for(auto& out : outputs){
//
//        UnclaimedTransaction::Builder builder;
//        builder.SetTransactionHash(Hash());
//        builder.SetTransactionIndex(index++);
//        builder.SetUser(out.user().ToString());
//        builder.SetProduct(out.product().ToString());
//
//        auto utxo = builder.Build();
//        auto hash = utxo->hash();
//
//        batch.PutUnclaimedTransaction(hash, utxo);
//      }
//    }

#ifdef TOKEN_DEBUG
    auto duration_ms = GetElapsedTimeMilliseconds(start, Clock::now());
    DLOG(INFO) << hash << " commit took " << duration_ms << "ms.";
#endif//TOKEN_DEBUG
  }

  bool AsyncBlockCommitter::Commit(const Hash& hash){

  }
}