#include "block_verifier_sync.h"

namespace token{
  namespace sync{
    bool TransactionVerifier::Visit(const Input& val){
      NOT_IMPLEMENTED(FATAL);//TODO: implement
      return false;
    }

    bool TransactionVerifier::Visit(const Output& val){
      NOT_IMPLEMENTED(FATAL);//TODO: implement
      return false;
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