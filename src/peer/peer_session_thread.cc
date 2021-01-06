#ifdef TOKEN_ENABLE_SERVER

#include "peer/peer_session_thread.h"
#include "peer/peer_session_manager.h"

namespace Token{
  std::string PeerSessionThread::GetStatusMessage(){
    std::stringstream ss;
    switch(GetState()){
      case PeerSessionThread::kStarting:ss << "Starting...";
        break;
      case PeerSessionThread::kIdle:ss << "Idle.";
        break;
      case PeerSessionThread::kRunning:{
        std::shared_ptr<PeerSession> session = GetCurrentSession();
        ss << session->GetState() << " " << session->GetInfo();
        break;
      }
      case PeerSessionThread::kStopped:ss << "Stopped.";
        break;
      default:ss << "Unknown!";
        break;
    }

    ss << " ";
    ss << "[" << GetStatus() << "]";
    return ss.str();
  }

  void PeerSessionThread::HandleThread(uword parameter){
    PeerSessionThread* thread = (PeerSessionThread*) parameter;
    thread->SetState(State::kStarting);

    char truncated_name[16];
    snprintf(truncated_name, 15, "PeerSession-%" PRId32, thread->GetWorkerID());
    pthread_setname_np(pthread_self(), truncated_name);

    // start-up logic here
    while(!thread->IsStopping()){
      thread->SetState(State::kIdle);
      PeerSessionManager::ConnectRequest request;
      if(PeerSessionManager::GetNextRequest(request, PeerSessionThread::kRequestTimeoutIntervalMilliseconds)){
        NodeAddress paddr = request.GetAddress();
        LOG(INFO) << "connecting to peer: " << paddr;
        std::shared_ptr<PeerSession> session = thread->CreateNewSession(paddr);
        thread->SetState(PeerSessionThread::kRunning);
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
    char name[16];
    snprintf(name, 16, "peer-%" PRId16, GetWorkerID());
    return Thread::StartThread(&thread_, name, &HandleThread, (uword) this);
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