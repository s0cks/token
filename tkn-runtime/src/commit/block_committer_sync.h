#ifndef TKN_SYNC_BLOCK_COMMITTER_H
#define TKN_SYNC_BLOCK_COMMITTER_H

#include "batch.h"
#include "block_committer.h"
#include "../../../Sources/token/transaction_indexed.h"

namespace token{
  namespace sync{
    class BlockCommitter;
    class TransactionCommitter : public InputVisitor,
                                 public OutputVisitor{
    private:
      std::shared_ptr<internal::WriteBatch> batch_;
      uint64_t transaction_idx_;
      Hash transaction_hash_;
      IndexedTransactionPtr transaction_;
      uint64_t input_idx_;
      uint64_t output_idx_;

      inline std::shared_ptr<internal::WriteBatch>
      batch() const{
        return batch_;
      }
    public:
      TransactionCommitter(std::shared_ptr<internal::WriteBatch> batch, const Hash& hash, IndexedTransactionPtr val):
          InputVisitor(),
          OutputVisitor(),
          batch_(std::move(batch)),
          transaction_idx_(val->index()),
          transaction_hash_(hash),
          transaction_(std::move(val)),
          input_idx_(0),
          output_idx_(0){}
      ~TransactionCommitter() override = default;

      Hash hash() const{
        return transaction_hash_;
      }

      IndexedTransactionPtr GetTransaction() const{
        return transaction_;
      }

      bool Visit(const InputPtr& val) override;
      bool Visit(const OutputPtr& val) override;
    };

    class BlockCommitter : public internal::BlockCommitterBase,
                           public IndexedTransactionVisitor{
    private:
      std::shared_ptr<internal::WriteBatch> batch_;
    public:
      explicit BlockCommitter(const BlockPtr& blk, UnclaimedTransactionPool& utxos):
        internal::BlockCommitterBase(blk, utxos),
        IndexedTransactionVisitor(),
        batch_(std::make_shared<internal::WriteBatch>()){
      }
      ~BlockCommitter() override = default;

      bool Visit(const IndexedTransactionPtr& tx) override;
      bool Commit() override;
    };
  }
}

#endif//TKN_SYNC_BLOCK_COMMITTER_H