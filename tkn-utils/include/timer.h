#ifndef TKN_TIMER_H
#define TKN_TIMER_H

#include <uv.h>
#include "common.h"

namespace token{
  typedef void (*TimerCallback)(void*);

  class Timer{
  private:
    TimerCallback callback_;
    uv_timer_t handle_;
    uint64_t timeout_;
    uint64_t repeat_;

    inline uv_timer_t*
    handle(){
      return &handle_;
    }

    void Tick(){
      DVLOG(2) << "tick.";
      callback_();
    }

    static void
    OnTick(uv_timer_t* handle){
      auto timer = (Timer*)handle->data = this;
      timer->Tick();
    }
  public:
    explicit Timer(TimerCallback callback, uv_loop_t* loop, const uint64_t& delay, const uint64_t& repeat):
      callback_(callback),
      handle_(),
      timeout_(delay),
      repeat_(repeat){
      handle_.data = this;
      CHECK_UVRESULT2(FATAL, uv_timer_init(loop, handle()), "cannot initialize timer handle");
    }
    Timer(const Timer& rhs) = delete;
    ~Timer(){
      //TODO: stop timer handle
    }

    bool Start(){
      VERIFY_UVRESULT2(ERROR, uv_timer_start(handle(), &OnTick, timeout_, repeat_), "cannot start the timer handle");
      return true;
    }

    bool Stop(){
      VERIFY_UVRESULT2(ERROR, uv_timer_stop(handle()), "cannot stop the timer handle");
      return true;
    }

    Timer& operator=(const Timer& rhs) = delete;
  };
}

#endif//TKN_TIMER_H