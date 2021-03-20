#include <chrono>
#include <thread>
#include "peer/peer_session_thread.h"
#include "peer/peer_session_manager.h"

namespace token{
  ConnectionRequest* PeerSessionThread::GetNextRequest(){
    ConnectionRequest* next = queue_.Pop();
    if(next)
      return next;
    ConnectionRequestQueue* queue = PeerSessionManager::GetRandomQueue();
    if(!queue){
      pthread_yield();
      return nullptr;
    }
    return queue->Steal();
  }

  static inline ConnectionAttemptCounter
  GetMaxAttempts(){
    return TOKEN_MAX_CONNECTION_ATTEMPTS;
  }

  static inline ConnectionAttemptCounter
  GetAttempt(const ConnectionAttemptCounter& counter){
    return GetMaxAttempts()-counter;
  }

  static inline std::string
  GetAttemptStatus(const ConnectionAttemptCounter& counter){
    std::stringstream ss;
    ss << "(" << GetAttempt(counter) << "/" << GetMaxAttempts() << ")";
    return ss.str();
  }

  static inline Duration
  GetAttemptBackoffMilliseconds(const ConnectionAttemptCounter& counter){
    int64_t total_seconds = TOKEN_CONNECTION_BACKOFF_SCALE*(1+GetAttempt(counter));
    return std::chrono::seconds(total_seconds);
  }

  void PeerSessionThread::HandleThread(uword parameter){
    PeerSessionThread* thread = (PeerSessionThread*)parameter;
    thread->SetState(State::kStartingState);
    // start-up logic here

    thread->SetState(State::kRunningState);
    while(thread->IsRunningState()){
      ConnectionRequest* request = thread->GetNextRequest();
      if(!request)
        continue;

      NodeAddress paddress = request->GetAddress();
      ConnectionAttemptCounter counter = request->GetNumberOfAttemptsRemaining();

      PeerSession* session = thread->CreateNewSession(request->GetAddress());
      if(!session->Connect())
        THREAD_LOG(WARNING) << "couldn't connect to " << paddress << " " << GetAttemptStatus(counter) << ".";

#ifdef TOKEN_DEBUG
      THREAD_LOG(INFO) << "session disconnected.";
#endif//TOKEN_DEBUG
      thread->ClearSession();
      if(request->CanReschedule()){
        Duration backoff = GetAttemptBackoffMilliseconds(counter);
#ifdef TOKEN_DEBUG
        THREAD_LOG(INFO) << "waiting for " << std::chrono::duration_cast<std::chrono::milliseconds>(backoff).count() << "ns (back-off)....";
#endif//TOKEN_DEBUG

        std::this_thread::sleep_for(backoff);

#ifdef TOKEN_DEBUG
        THREAD_LOG(INFO) << "rescheduling connection to " << paddress << " " << GetAttemptStatus(counter) << "....";
#endif//TOKEN_DEBUG
        if(!thread->Schedule(paddress, counter - 1))
          THREAD_LOG(ERROR) << "couldn't schedule new connection to " << paddress << ".";
        continue;
      } else{
#ifdef TOKEN_DEBUG
        THREAD_LOG(INFO) << "not rescheduling connection to " << paddress << ".";
#endif//TOKEN_DEBUG
        continue;
      }
    }
    pthread_exit(0);
  }

  bool PeerSessionThread::Start(){
    char name[16];
    snprintf(name, 15, "peer-%" PRId8, id_);
    return ThreadStart(
      &thread_,
      name,
      &HandleThread,
      (uword)this
    );
  }

  bool PeerSessionThread::Shutdown(){
    SetState(PeerSessionThread::kStoppingState);
    if(IsConnected() && !DisconnectSession()){
      THREAD_LOG(ERROR) << "cannot disconnect from peer.";
      return false;
    }
    return true;
  }

  bool PeerSessionThread::WaitForShutdown(){
    return ThreadJoin(thread_);
  }
}