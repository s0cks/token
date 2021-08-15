#include "block_verifier_sync.h"

namespace token{
  namespace sync{
    bool TransactionVerifier::Visit(const Input& val){
      
    }

    bool TransactionVerifier::Visit(const Output& val){
      NOT_IMPLEMENTED(FATAL);//TODO: implement
      return false;
    }

    bool TransactionVerifier::Verify(){
      if(!transaction_->VisitInputs(this)){
        LOG(ERROR) << "cannot visit transaction " << hash() << " inputs.";
        return false;
      }

      if(!transaction_->VisitOutputs(this)){
        LOG(ERROR) << "cannot visit transaction " << hash() << " outputs.";
        return false;
      }
      return IsValid();
    }

    bool BlockVerifier::Visit(const IndexedTransactionPtr& val){
      DVLOG(2) << "visiting " << val->ToString() << "....";
      TransactionVerifier verifier(utxos(), val);

    }

    bool BlockVerifier::Verify(){
      DLOG(INFO) << "(sync) verifying block " << hash() << "....";
      auto start = Clock::now();

      uint64_t idx = 0;
      for(; idx < num_transactions_; idx++){
        auto tx = block()->GetTransaction(idx);
      }

      DLOG(INFO) << "(sync) commit block " << hash() << " has finished, took " << GetElapsedTimeMilliseconds(start) << "ms.";
      return true;
    }
  }
}