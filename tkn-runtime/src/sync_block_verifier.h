#ifndef TKN_SYNC_BLOCK_VERIFIER_H
#define TKN_SYNC_BLOCK_VERIFIER_H

#include "block_verifier.h"
#include "atomic/bit_vector.h"

namespace token{
  namespace sync{
    class BlockVerifier : public internal::BlockVerifierBase{
    protected:
      uint64_t num_transactions_;
      atomic::BitVector results_;
    public:
      BlockVerifier(const Hash& hash, const BlockPtr& val):
        internal::BlockVerifierBase(hash, val),
        num_transactions_(val->GetNumberOfTransactions()),
        results_(val->GetNumberOfTransactions()){}
      ~BlockVerifier() override = default;

      atomic::BitVector& GetResults(){
        return results_;
      }

      bool Verify() override;
    };
  }
}

#endif//TKN_SYNC_BLOCK_VERIFIER_H