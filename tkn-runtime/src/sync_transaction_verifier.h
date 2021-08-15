#ifndef TKN_SYNC_TRANSACTION_VERIFIER_H
#define TKN_SYNC_TRANSACTION_VERIFIER_H

#include "object_pool.h"
#include "transaction_indexed.h"

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
  }
}

#endif//TKN_SYNC_TRANSACTION_VERIFIER_H