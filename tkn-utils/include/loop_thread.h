#ifndef TKN_LOOP_THREAD_H
#define TKN_LOOP_THREAD_H

#include <uv.h>
#include <glog/logging.h>

#include "os_thread.h"

namespace token{
  template<class Looper, const uv_run_mode RunMode=UV_RUN_DEFAULT>
  class LoopThread{
  private:
    ThreadId thread_;
    uv_loop_t* loop_;

    static void HandleThread(uword parameter){
      auto loop = (uv_loop_t*)parameter;

      int err;
      if((err = uv_run(loop, RunMode)) != 0){
        DLOG(ERROR) << "cannot run the " << Looper::GetThreadName() << " loop: " << uv_strerror(err);
        return pthread_exit(nullptr);
      }

      DLOG(INFO) << "the " << Looper::GetThreadName() << " has finished.";
      return pthread_exit(nullptr);
    }
  public:
    explicit LoopThread(uv_loop_t* loop=uv_loop_new()):
      thread_(),
      loop_(loop){}
    ~LoopThread(){
      uv_loop_delete(loop_);
    }

    uv_loop_t* loop() const{
      return loop_;
    }

    bool Start(){
      return platform::ThreadStart(&thread_, Looper::GetThreadName(), &HandleThread, (uword)loop_);
    }

    bool Join(){
      return platform::ThreadJoin(thread_);
    }
  };
}

#endif//TKN_LOOP_THREAD_H