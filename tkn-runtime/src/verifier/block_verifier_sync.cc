#include "block_verifier_sync.h"

namespace token{
  namespace sync{
    bool TransactionInputVerifier::IsValid(const Input& val){
      auto hash = val.utxo_hash();
      if(!pool().Has(hash)){
        LOG(ERROR) << "input #" << (processed()+1) << " is not valid, cannot find unclaimed transaction: " << hash << ".";
        return false;
      }
      DVLOG(1) << "input #" << (processed()+1) << " is valid.";
      return true;
    }

    bool TransactionOutputVerifier::Visit(const Output& val){//TODO: implement?
      processed_++;
      valid_++;
      return true;
    }

    bool TransactionVerifier::Verify(){
      if(!transaction_->VisitInputs(&input_verifier_)){
        LOG(ERROR) << "cannot visit transaction " << hash() << " inputs.";
        return false;
      }

      if(!transaction_->VisitOutputs(&output_verifier_)){
        LOG(ERROR) << "cannot visit transaction " << hash() << " outputs.";
        return false;
      }
      return IsValid();
    }

    bool TransactionVerifier::IsValid(){
      return input_verifier_.invalid() == 0;
    }

    bool BlockVerifier::Visit(const IndexedTransactionPtr& val){
      auto hash = val->hash();
      DVLOG(2) << "visiting " << val->ToString() << "....";

      return true;
    }

    bool BlockVerifier::Verify(){
      DLOG(INFO) << "(sync) verifying block " << hash() << "....";
      auto start = Clock::now();

      uint64_t idx = 0;
      for(; idx < num_transactions_; idx++){
        auto tx = block()->GetTransaction(idx);
        TransactionVerifier verifier(utxos(), tx);
        if(!verifier.Verify()){
          LOG(ERROR) << "transaction " << verifier.hash() << " is invalid: ";
          LOG(ERROR) << "  - inputs valid=" << verifier.input_verifier().GetPercentageValid() << "%, invalid=" << verifier.input_verifier().GetPercentageInvalid() << "%";
          LOG(ERROR) << "  - outputs valid=" << verifier.output_verifier().GetPercentageValid() << "%, invalid=" << verifier.output_verifier().GetPercentageInvalid() << "%";
          return true;
        }
        DVLOG(1) << "transaction " << verifier.hash() << " is valid.";
      }

      DLOG(INFO) << "(sync) verify block " << hash() << " has finished, took " << GetElapsedTimeMilliseconds(start) << "ms.";
      return true;
    }
  }
}