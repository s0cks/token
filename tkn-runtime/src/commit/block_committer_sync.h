#ifndef TKN_SYNC_BLOCK_COMMITTER_H
#define TKN_SYNC_BLOCK_COMMITTER_H

#include "batch.h"
#include "block_committer.h"
#include "transaction_indexed.h"

namespace token{
  namespace sync{
    class BlockCommitter;
    class TransactionCommitter : public InputVisitor,
                                 public OutputVisitor{
    private:
      UnclaimedTransactionPool::WriteBatch* batch_;
      uint64_t transaction_idx_;
      Hash transaction_hash_;
      IndexedTransactionPtr transaction_;
      uint64_t input_idx_;
      uint64_t output_idx_;

      inline UnclaimedTransactionPool::WriteBatch*
      batch() const{
        return batch_;
      }
    public:
      TransactionCommitter(UnclaimedTransactionPool::WriteBatch* batch, const Hash& hash, IndexedTransactionPtr val):
          InputVisitor(),
          OutputVisitor(),
          batch_(batch),
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

      bool Visit(const Input& val) override;
      bool Visit(const Output& val) override;
    };

    class BlockCommitter : public internal::BlockCommitterBase,
                           public IndexedTransactionVisitor{
    private:
      UnclaimedTransactionPool::WriteBatch batch_;
    public:
      explicit BlockCommitter(const BlockPtr& blk, UnclaimedTransactionPool& utxos):
        internal::BlockCommitterBase(blk, utxos),
        IndexedTransactionVisitor(),
        batch_(){}
      ~BlockCommitter() override = default;

      bool Visit(const IndexedTransactionPtr& tx) override;
      bool Commit() override;
    };
  }
}

#endif//TKN_SYNC_BLOCK_COMMITTER_H