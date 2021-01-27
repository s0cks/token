#ifdef TOKEN_ENABLE_SERVER

#include "peer/peer_session_thread.h"
#include "peer/peer_session_manager.h"

namespace Token{
  void PeerSessionThread::HandleThread(uword parameter){
    PeerSessionThread* thread = (PeerSessionThread*) parameter;
    thread->SetState(State::kStarting);

    thread->SetState(State::kRunning);
    // start-up logic here
    while(!thread->IsStopping()){
      PeerSessionManager::ConnectRequest request;
      if(PeerSessionManager::GetNextRequest(request, PeerSessionThread::kRequestTimeoutIntervalMilliseconds)){
        NodeAddress paddr = request.GetAddress();
        LOG(INFO) << "[" << thread->GetThreadName() << "] connecting to peer: " << paddr;
        PeerSession* session = thread->CreateNewSession(paddr);
        if(!session->Connect()){
          if(request.ShouldReschedule()){
            int32_t backoffs = request.GetNumberOfAttempts() * PeerSessionManager::kRetryBackoffSeconds;
            LOG(WARNING) << "couldn't connect to peer " << paddr << ", rescheduling (" << backoffs << "s)...";
            sleep(backoffs);
            if(!PeerSessionManager::RescheduleRequest(request))
              LOG(WARNING) << "couldn't reschedule peer: " << paddr;
          } else{
            LOG(WARNING) << "couldn't connect to peer " << paddr << ".";
          }
          continue;
        }

        thread->ClearCurrentSession();
        if(session->IsError() && request.ShouldReschedule()){
          if(request.ShouldReschedule()){
            int32_t backoffs = request.GetNumberOfAttempts() * PeerSessionManager::kRetryBackoffSeconds;
            LOG(WARNING) << "couldn't connect to peer " << paddr << ", rescheduling (" << backoffs << "s)...";
            sleep(backoffs);
            if(!PeerSessionManager::RescheduleRequest(request))
              LOG(WARNING) << "couldn't reschedule peer: " << paddr;
          } else{
            LOG(WARNING) << "couldn't connect to peer " << paddr << ".";
          }
        }
        LOG(INFO) << "disconnected from peer: " << paddr;
      }
    }
    pthread_exit(0);
  }

  bool PeerSessionThread::Start(){
    return Thread::StartThread(&thread_, thread_name_.data(), &HandleThread, (uword) this);
  }

  bool PeerSessionThread::Stop(){
    SetState(PeerSessionThread::kStopping);

    void** result = NULL;
    int err;
    if((err = pthread_join(thread_, result)) != 0){
      LOG(WARNING) << "couldn't join thread: " << strerror(err);
      return false;
    }
    return true;
  }
}

#endif//TOKEN_ENABLE_SERVER