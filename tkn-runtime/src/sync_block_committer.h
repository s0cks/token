#ifndef TKN_SYNC_BLOCK_COMMITTER_H
#define TKN_SYNC_BLOCK_COMMITTER_H

#include "block_committer.h"
#include "transaction_indexed.h"

namespace token{
  namespace sync{
    class BlockCommitter : public internal::BlockCommitterBase,
                           public IndexedTransactionVisitor{
    protected:
      Hash block_;
    public:
      explicit BlockCommitter():
        internal::BlockCommitterBase(),
        IndexedTransactionVisitor(),
        block_(){}
      ~BlockCommitter() override = default;

      bool Visit(const IndexedTransactionPtr& tx) override;
      bool Commit(const Hash& hash) override;
    };
  }
}

#endif//TKN_SYNC_BLOCK_COMMITTER_H