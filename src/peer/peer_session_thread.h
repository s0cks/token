#ifndef TOKEN_PEER_SESSION_THREAD_H
#define TOKEN_PEER_SESSION_THREAD_H

#ifdef TOKEN_ENABLE_SERVER

#include <mutex>
#include <queue>
#include <memory>
#include <iostream>
#include <condition_variable>

#include "vthread.h"
#include "peer/peer_session.h"

namespace Token{
  typedef int16_t WorkerId;

#define FOR_EACH_PEER_SESSION_STATE(V) \
    V(Starting)                                \
    V(Idle)                                    \
    V(Running)                                 \
    V(Stopping)                                \
    V(Stopped)

#define FOR_EACH_PEER_SESSION_STATUS(V) \
    V(Ok)                                       \
    V(Warning)                                  \
    V(Error)

  class PeerSessionThread{
    friend class PeerSessionManager;
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_PEER_SESSION_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_PEER_SESSION_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }

    enum Status{
#define DEFINE_STATUS(Name) k##Name,
      FOR_EACH_PEER_SESSION_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_PEER_SESSION_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }

    static const int64_t kRequestTimeoutIntervalMilliseconds = 1000;
   private:
    std::string thread_name_;
    ThreadId thread_;
    WorkerId worker_;

    std::mutex mutex_;
    std::condition_variable cond_;
    State state_;
    Status status_;
    uv_loop_t* loop_;
    PeerSession* session_;

    PeerSession* CreateNewSession(const NodeAddress& address){
      std::lock_guard<std::mutex> guard(mutex_);
      session_ = new PeerSession(GetLoop(), address);
      return session_;
    }

    static inline std::string
    GetWorkerThreadName(const WorkerId& worker){
      std::stringstream ss;
      ss << "peer-" << worker;
      return ss.str();
    }

    static void HandleThread(uword parameter);
   public:
    PeerSessionThread(const WorkerId& worker, uv_loop_t* loop = uv_loop_new()):
      thread_name_(GetWorkerThreadName(worker)),
      thread_(),
      worker_(worker),
      mutex_(),
      cond_(),
      state_(State::kStopped),
      status_(Status::kOk),
      loop_(loop),
      session_(nullptr){}
    ~PeerSessionThread(){
      if(loop_){
        uv_loop_delete(loop_);
      }
    }

    std::string GetThreadName() const{
      return thread_name_;
    }

    ThreadId GetThreadID() const{
      return thread_;
    }

    WorkerId GetWorkerID() const{
      return worker_;
    }

    uv_loop_t* GetLoop() const{
      return loop_;
    }

    State GetState(){
      std::lock_guard<std::mutex> guard(mutex_);
      return state_;
    }

    void SetState(const State& state){
      std::lock_guard<std::mutex> guard(mutex_);
      state_ = state;
    }

    Status GetStatus(){
      std::lock_guard<std::mutex> guard(mutex_);
      return status_;
    }

    void SetStatus(const Status& status){
      std::lock_guard<std::mutex> guard(mutex_);
      status_ = status;
    }

    PeerSession* GetCurrentSession(){
      std::lock_guard<std::mutex> guard(mutex_);
      return session_;
    }

    bool HasSession(){
      std::lock_guard<std::mutex> guard(mutex_);
      return session_ != nullptr;
    }

    bool Start();
    bool Stop();
#define DEFINE_CHECK(Name) \
        bool Is##Name(){ return GetState() == State::k##Name; }
    FOR_EACH_PEER_SESSION_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK

#define DEFINE_CHECK(Name) \
        bool Is##Name(){ return GetStatus() == Status::k##Name; }
    FOR_EACH_PEER_SESSION_STATUS(DEFINE_CHECK)
#undef DEFINE_CHECK
  };
}

#endif//TOKEN_ENABLE_SERVER
#endif //TOKEN_PEER_SESSION_THREAD_H