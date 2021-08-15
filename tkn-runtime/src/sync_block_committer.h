#ifndef TKN_SYNC_BLOCK_COMMITTER_H
#define TKN_SYNC_BLOCK_COMMITTER_H

#include "block_committer.h"
#include "transaction_indexed.h"

namespace token{
  namespace sync{
    class BlockCommitter : public internal::BlockCommitterBase,
                           public IndexedTransactionVisitor{
    private:
      internal::WriteBatch batch_;
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