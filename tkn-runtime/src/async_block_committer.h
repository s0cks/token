#ifndef TKN_ASYNC_BLOCK_COMMITTER_H
#define TKN_ASYNC_BLOCK_COMMITTER_H

#include "block_committer.h"

namespace token{
  namespace async{
    class BlockCommitter : public internal::BlockCommitterBase{
    public:
      BlockCommitter():
        internal::BlockCommitterBase(){}
      ~BlockCommitter() override = default;

      bool Commit(const Hash& hash) override;
    };
  }
}

#endif//TKN_ASYNC_BLOCK_COMMITTER_H