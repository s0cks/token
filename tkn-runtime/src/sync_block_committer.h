#ifndef TKN_SYNC_BLOCK_COMMITTER_H
#define TKN_SYNC_BLOCK_COMMITTER_H

#include "block_committer.h"
#include "transaction_indexed.h"

namespace token{
  namespace sync{
    class BlockCommitter : public internal::BlockCommitterBase,
                           public IndexedTransactionVisitor{
    protected:
      ObjectPool& pool_;
      internal::PoolWriteBatch batch_;
      Hash block_;
    public:
      explicit BlockCommitter(ObjectPool& pool):
        internal::BlockCommitterBase(),
        IndexedTransactionVisitor(),
        pool_(pool),
        batch_(),
        block_(){}
      ~BlockCommitter() override = default;

      ObjectPool& GetObjectPool(){
        return pool_;
      }

      bool Visit(const IndexedTransactionPtr& tx) override;
      bool Commit(const Hash& hash) override;
    };
  }
}

#endif//TKN_SYNC_BLOCK_COMMITTER_H