#ifndef TOKEN_PEER_SESSION_THREAD_H
#define TOKEN_PEER_SESSION_THREAD_H

#include <mutex>
#include <queue>
#include <memory>
#include <iostream>
#include <condition_variable>

#include "os_thread.h"
#include "peer/peer_queue.h"
#include "peer/peer_session.h"

namespace token{
  typedef int8_t WorkerId;

#define FOR_EACH_PEER_SESSION_STATE(V) \
    V(Starting)                                \
    V(Idle)                                    \
    V(Running)                                 \
    V(Stopping)                                \
    V(Stopped)

  class PeerSessionThread{
   public:
    enum State{
#define DEFINE_STATE(Name) k##Name##State,
      FOR_EACH_PEER_SESSION_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name##State: \
          return stream << #Name;
        FOR_EACH_PEER_SESSION_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

    static const int64_t kRequestTimeoutIntervalMilliseconds = 1000;
   private: // loop_ needs to be created on thread
    ThreadId thread_;
    ConnectionRequestQueue queue_;
    atomic::RelaxedAtomic<State> state_;
    WorkerId id_;

    std::mutex session_mtx_;
    peer::Session* session_;

    void SetState(const State& state){
      state_ = state;
    }

    void SetSession(peer::Session* session){
      std::lock_guard<std::mutex> guard(session_mtx_);
      session_ = session;
    }

    bool Schedule(const utils::Address& address, const ConnectionAttemptCounter attempts){
      return queue_.Push(new ConnectionRequest(address, attempts));
    }

    inline void
    ClearSession(){
      return SetSession(nullptr);
    }

    bool DisconnectSession(){
      if(!IsConnected())
        return false;
      std::lock_guard<std::mutex> guard(session_mtx_);
      return session_->Disconnect();
    }

    peer::Session* CreateNewSession(const utils::Address& address){
      auto session = new peer::Session(address);
      SetSession(session);
      return session;
    }

    ConnectionRequest* GetNextRequest();
    static void HandleThread(uword param);
   public:
    explicit PeerSessionThread(const WorkerId& worker_id):
      thread_(),
      queue_(TOKEN_CONNECTION_QUEUE_SIZE),
      state_(State::kStoppedState),
      id_(worker_id),
      session_mtx_(),
      session_(nullptr){}
    ~PeerSessionThread() = default;

    ThreadId GetThreadId() const{
      return thread_;
    }

    State GetState() const{
      return (State)state_;
    }

    WorkerId GetWorkerId() const{
      return id_;
    }

    ConnectionRequestQueue* GetQueue(){
      return &queue_;
    }

    bool Start();
    bool Shutdown();
    bool WaitForShutdown(); //TODO: add timeout?

    bool IsConnected(){
      std::lock_guard<std::mutex> guard(session_mtx_);
      return session_ != nullptr;
    }

    peer::Session* GetPeerSession(){
      std::lock_guard<std::mutex> guard(session_mtx_);
      return session_;
    }

    utils::Address GetPeerAddress(){
      std::lock_guard<std::mutex> guard(session_mtx_);
      return session_->address();
    }

    UUID GetPeerID(){
      std::lock_guard<std::mutex> guard(session_mtx_);
      return UUID();//TODO: implement
    }

#define DEFINE_STATE_CHECK(Name) \
    inline bool Is##Name##State() const{ return GetState() == State::k##Name##State; }
    FOR_EACH_PEER_SESSION_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };
}

#endif //TOKEN_PEER_SESSION_THREAD_H