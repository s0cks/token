#ifndef TOKEN_UTILS_TIMER_H
#define TOKEN_UTILS_TIMER_H

#include <uv.h>

#include "common.h"

namespace token{
  namespace utils{
    typedef void (*TimerCallback)();

    class Timer{
     protected:
      uv_timer_t handle_;
      TimerCallback callback_;
      int64_t initial_;
      int64_t repeat_;

      static inline void
      OnTick(uv_timer_t* handle){
        auto instance = (Timer*)handle->data;
        return instance->callback_();
      }
     public:
      explicit Timer(uv_loop_t* loop, const TimerCallback& callback, const int64_t& initial, const int64_t& repeat):
        handle_(),
        callback_(callback),
        initial_(initial),
        repeat_(repeat){
        handle_.data = this;
        CHECK_UVRESULT2(ERROR, uv_timer_init(loop, &handle_), "couldn't initialize timer handle");
      }
      ~Timer() = default;

      bool Start(){
        VERIFY_UVRESULT2(ERROR, uv_timer_start(&handle_, &OnTick, initial_, repeat_), "couldn't start timer handle");
        return true;
      }

      bool Stop(){
        VERIFY_UVRESULT2(ERROR, uv_timer_stop(&handle_), "couldn't stop timer handle");
        return true;
      }
    };
  }
}

#endif//TOKEN_UTILS_TIMER_H