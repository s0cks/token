#ifndef TKN_SYNC_BLOCK_VERIFIER_H
#define TKN_SYNC_BLOCK_VERIFIER_H

#include "object_pool.h"
#include "block_verifier.h"
#include "atomic/bit_vector.h"

namespace token{
  namespace sync{
    class TransactionVerifier : public InputVisitor,
                                public OutputVisitor{
    private:
      UnclaimedTransactionPool& pool_;
      Hash transaction_hash_;
      uint64_t transaction_idx_;
      uint64_t input_idx_;
      uint64_t output_idx_;

      inline UnclaimedTransactionPool&
      pool() const{
        return pool_;
      }
    public:
      explicit TransactionVerifier(UnclaimedTransactionPool& pool, const IndexedTransactionPtr& val):
          InputVisitor(),
          OutputVisitor(),
          pool_(pool),
          transaction_hash_(val->hash()),
          transaction_idx_(val->index()),
          input_idx_(0),
          output_idx_(0){}
      ~TransactionVerifier() override = default;

      bool Visit(const Input& val) override;
      bool Visit(const Output& val) override;
    };

    class BlockVerifier : public internal::BlockVerifierBase{
    protected:
      BlockPtr block_;
      uint64_t num_transactions_;
      atomic::BitVector results_;
    public:
      BlockVerifier(const BlockPtr& blk, UnclaimedTransactionPool& utxos):
        internal::BlockVerifierBase(blk, utxos),
        num_transactions_(blk->GetNumberOfTransactions()),
        results_(blk->GetNumberOfTransactions()){}
      ~BlockVerifier() override = default;

      atomic::BitVector& GetResults(){
        return results_;
      }

      bool Verify() override;
    };
  }
}

#endif//TKN_SYNC_BLOCK_VERIFIER_H