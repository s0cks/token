#ifdef TOKEN_ENABLE_SERVER

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

  static inline std::string
  GetCurrentAttempt(const ConnectionAttemptCounter& attempt){
    ConnectionAttemptCounter current = TOKEN_MAX_CONNECTION_ATTEMPTS-attempt;
    std::stringstream ss;
    ss << current << "/" << TOKEN_MAX_CONNECTION_ATTEMPTS;
    return ss.str();
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
      ConnectionAttemptCounter attempt = request->GetNumberOfAttemptsRemaining();

      PeerSession* session = thread->CreateNewSession(request->GetAddress());
      if(!session->Connect())
        THREAD_LOG(WARNING) << "couldn't connect to " << paddress << " (" << GetCurrentAttempt(attempt) << ").";

#ifdef TOKEN_DEBUG
      THREAD_LOG(INFO) << "session disconnected.";
#endif//TOKEN_DEBUG
      thread->ClearSession();

      THREAD_LOG(INFO) << "attempts remaining: " << request->GetNumberOfAttemptsRemaining();
      if(request->CanReschedule()){
#ifdef TOKEN_DEBUG
        THREAD_LOG(INFO) << "rescheduling connection to " << paddress << " (" << GetCurrentAttempt(attempt) << ")....";
#endif//TOKEN_DEBUG
        if(!thread->Schedule(paddress, --attempt)){
          THREAD_LOG(ERROR) << "couldn't schedule new connection to " << paddress;
          continue;
        }
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

#endif//TOKEN_ENABLE_SERVER