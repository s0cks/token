#ifndef TKN_ASYNC_BLOCK_COMMITTER_H
#define TKN_ASYNC_BLOCK_COMMITTER_H

#include "block_committer.h"

namespace token{
  namespace async{
    class BlockCommitter : public internal::BlockCommitterBase{
    public:
      BlockCommitter(const BlockPtr& blk, UnclaimedTransactionPool& utxos):
        internal::BlockCommitterBase(blk, utxos){}
      ~BlockCommitter() override = default;

      bool Commit() override;
    };
  }
}

#endif//TKN_ASYNC_BLOCK_COMMITTER_H