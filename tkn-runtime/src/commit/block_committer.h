#ifndef TKN_BLOCK_COMMITTER_H
#define TKN_BLOCK_COMMITTER_H

#include <uv.h>

#include "batch.h"
#include "eventbus.h"
#include "object_pool.h"
#include "atomic/bit_vector.h"

namespace token{
#define FOR_EACH_BLOCK_COMMITTER_EVENT(V) \
  V(BlockCommitted, "commit.block", on_block_commit_)

  class BlockCommitListener{
  protected:
    uv_async_t on_block_commit_;

    BlockCommitListener(uv_loop_t* loop, EventBus& bus):
      on_block_commit_(){
#define INITIALIZE_EVENT_HANDLE(Name, Event, Handle) \
      (Handle).data = this;                          \
      CHECK_UVRESULT2(FATAL, uv_async_init(loop, &(Handle), [](uv_async_t* handle){ \
        return ((BlockCommitListener*)handle->data)->HandleOn##Name();              \
      }), "cannot initialize the callback handle");  \
      bus.Subscribe(Event, &(Handle));
      FOR_EACH_BLOCK_COMMITTER_EVENT(INITIALIZE_EVENT_HANDLE)
#undef INITIALIZE_EVENT_HANDLE
    }

#define DECLARE_EVENT_HANDLER(Name, Event, Handle) \
    virtual void HandleOn##Name() = 0;
    FOR_EACH_BLOCK_COMMITTER_EVENT(DECLARE_EVENT_HANDLER)
#undef DECLARE_EVENT_HANDLER
  public:
    virtual ~BlockCommitListener() = default;
  };

#define DECLARE_BLOCK_COMMITTER_EVENT_HANDLER(Name, Event, Handle) \
  void HandleOn##Name() override;
#define DEFINE_BLOCK_COMMITTER_EVENT_LISTENER \
  FOR_EACH_BLOCK_COMMITTER_EVENT(DECLARE_BLOCK_COMMITTER_EVENT_HANDLER)

  namespace internal{
    class BlockCommitterBase{
    protected:
      BlockPtr block_;
      Hash block_hash_;
      UnclaimedTransactionPool& utxos_;

      BlockCommitterBase(const BlockPtr& blk, UnclaimedTransactionPool& utxos):
        block_(blk),
        block_hash_(blk->hash()),
        utxos_(utxos){}

      inline UnclaimedTransactionPool&
      utxos() const{
        return utxos_;
      }
    public:
      virtual ~BlockCommitterBase() = default;

      BlockPtr GetBlock() const{
        return block_;
      }

      Hash GetBlockHash() const{
        return block_hash_;
      }

      virtual bool Commit() = 0;
    };
  }
}

#endif//TKN_BLOCK_COMMITTER_H