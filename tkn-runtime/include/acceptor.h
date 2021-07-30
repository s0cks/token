#ifndef TKN_ACCEPTOR_H
#define TKN_ACCEPTOR_H

namespace token{
  class Runtime;
  class Acceptor{
#ifdef TOKEN_DEBUG
    static const uint64_t kTimeoutMilliseconds = 1000 * 30; // 30 seconds
#else
    static const uint64_t kTimeoutMilliseconds = 1000 * 60 * 60 * 10; // 10 minutes
#endif//TOKEN_DEBUG
  private:
    Runtime* runtime_;
    uv_async_t on_prepare_;
    uv_async_t on_commit_;
    uv_async_t on_accepted_;
    uv_async_t on_rejected_;
    uv_timer_t timeout_;

    static void HandleOnPrepare(uv_async_t* handle);
    static void HandleOnCommit(uv_async_t* handle);
    static void HandleOnAccepted(uv_async_t* handle);
    static void HandleOnRejected(uv_async_t* handle);
    static void HandleOnTimeout(uv_timer_t* handle);
  public:
    explicit Acceptor(Runtime* runtime);
    ~Acceptor() = default;

    Runtime* GetRuntime() const{
      return runtime_;
    }

    bool OnPrepare(){
      VERIFY_UVRESULT2(FATAL, uv_async_send(&on_prepare_), "cannot invoke on_prepare_ callback");
      return true;
    }

    bool OnCommit(){
      VERIFY_UVRESULT2(FATAL, uv_async_send(&on_commit_), "cannot invoke on_commit_ callback");
      return true;
    }

    bool OnAccepted(){
      VERIFY_UVRESULT2(FATAL, uv_async_send(&on_accepted_), "cannot invoke on_accepted_ callback");
      return true;
    }

    bool OnRejected(){
      VERIFY_UVRESULT2(FATAL, uv_async_send(&on_rejected_), "cannot invoke on_rejected_ callback");
      return true;
    }

    bool StartTimeout(){
      VERIFY_UVRESULT2(FATAL, uv_timer_start(&timeout_, &HandleOnTimeout, kTimeoutMilliseconds, 0), "cannot start timeout_ timer handle");
      return true;
    }

    bool StopTimeout(){
      VERIFY_UVRESULT2(FATAL, uv_timer_stop(&timeout_), "cannot stop timeout_ timer handle");
      return true;
    }
  };
}

#endif//TKN_ACCEPTOR_H