#ifndef TKN_BLOCK_COMMITTER_H
#define TKN_BLOCK_COMMITTER_H

#include <uv.h>

#include "batch.h"
#include "eventbus.h"

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

  class BlockCommitter{
  protected:
    BlockCommitter() = default;
  public:
    virtual ~BlockCommitter() = default;
    virtual bool Commit(const Hash& hash) = 0;
  };

  class SyncBlockCommitter : public BlockCommitter,
                             public IndexedTransactionVisitor,
                             public InputVisitor,
                             public OutputVisitor{
  protected:
    internal::PoolWriteBatch batch_;
    Hash block_;
    IndexedTransactionPtr transaction_;
  public:
    SyncBlockCommitter():
      BlockCommitter(),
      IndexedTransactionVisitor(),
      batch_(),
      block_(),
      transaction_(){}
    ~SyncBlockCommitter() override = default;

    bool Visit(const Input& val) override;
    bool Visit(const Output& val) override;
    bool Visit(const IndexedTransactionPtr& tx) override;
    bool Commit(const Hash& hash) override;
  };

  class AsyncBlockCommitter : public BlockCommitter{
  public:
    AsyncBlockCommitter():
      BlockCommitter(){}
    ~AsyncBlockCommitter() override = default;

    bool Commit(const Hash& hash) override;
  };
}

#endif//TKN_BLOCK_COMMITTER_H