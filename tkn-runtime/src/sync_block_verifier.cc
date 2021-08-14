#include "sync_block_verifier.h"
#include "sync_transaction_verifier.h"

namespace token{
  namespace sync{
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