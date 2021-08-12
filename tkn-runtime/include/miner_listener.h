#ifndef TKN_MINER_LISTENER_H
#define TKN_MINER_LISTENER_H

#include <uv.h>

namespace token{
#define FOR_EACH_MINER_EVENT(V) \
  V(Mine, on_mine_)

  class MinerEventListener{
  protected:
    uv_async_t on_mine_;
#define DECLARE_EVENT_HANDLER(Name, Handle) \
    virtual void HandleOn##Name() = 0;
    FOR_EACH_MINER_EVENT(DECLARE_EVENT_HANDLER)
#undef DECLARE_EVENT_HANDLER
  public:
    explicit MinerEventListener(uv_loop_t* loop):
      on_mine_(){
#define INITIALIZE_EVENT_HANDLE(Name, Handle) \
      (Handle).data = this;             \
      CHECK_UVRESULT2(FATAL, uv_async_init(loop, &(Handle), [](uv_async_t* handle){ \
        return ((MinerEventListener*)handle->data)->HandleOn##Name();                    \
      }), "cannot initialize the callback handle");
      FOR_EACH_MINER_EVENT(INITIALIZE_EVENT_HANDLE)
#undef INITIALIZE_EVENT_HANDLE
    }
    virtual ~MinerEventListener() = default;

#define DEFINE_ON_EVENT(Name, Handle) \
    bool On##Name(){ \
      VERIFY_UVRESULT2(FATAL, uv_async_send(&(Handle)), "cannot invoke the callback handle"); \
      return true;                    \
    }
    FOR_EACH_MINER_EVENT(DEFINE_ON_EVENT)
#undef DEFINE_ON_EVENT
  };

#define DECLARE_MINER_EVENT_HANDLER(Name, Handle) \
  void HandleOn##Name() override;

#define DEFINE_MINER_EVENT_LISTENER \
  FOR_EACH_MINER_EVENT(DECLARE_MINER_EVENT_HANDLER)
}

#endif//TKN_MINER_LISTENER_H