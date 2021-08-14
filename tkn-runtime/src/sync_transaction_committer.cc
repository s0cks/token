#include "sync_transaction_committer.h"
#include "transaction_unclaimed.h"

namespace token{
  namespace sync{
    bool TransactionCommitter::Visit(const Input& val){
      //TODO: remove inputs

      NOT_IMPLEMENTED(ERROR);//TODO: implement
      return true;
    }

    bool TransactionCommitter::Visit(const Output& val){
      auto index = output_idx_;

      DVLOG(2) << "visiting output #" << index << " " << val.ToString() << "....";
      UnclaimedTransaction::Builder builder;
      builder.SetTransactionHash(hash());
      builder.SetTransactionIndex(index);
      builder.SetProduct(val.product().ToString());
      builder.SetUser(val.user().ToString());

      auto utxo = builder.Build();
      auto utxo_hash = utxo->hash();
      DVLOG(1) << "created unclaimed transaction " << utxo_hash << " from " << TransactionReference(hash(), index);
      batch_->PutUnclaimedTransaction(utxo_hash, utxo);
      return true;
    }
  }
}