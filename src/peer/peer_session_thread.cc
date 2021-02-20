#ifdef TOKEN_ENABLE_SERVER

#include "peer/peer_session_thread.h"
#include "peer/peer_session_manager.h"

namespace token{
  void PeerSessionThread::HandleThread(uword parameter){
    PeerSessionThread* thread = (PeerSessionThread*) parameter;
    thread->SetState(State::kStarting);
    // start-up logic here
    uv_async_init(thread->GetLoop(), &thread->disconnect_, &OnDisconnect);

    thread->SetState(State::kRunning);
    while(thread->IsRunning()){
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
          thread->ClearCurrentSession();
          continue;
        }

        int err;
        if((err = uv_run(thread->GetLoop(), UV_RUN_DEFAULT)) != 0){
          LOG(WARNING) << "couldn't run peer session loop: " << uv_strerror(err);
          continue; //TODO: better error handling
        }

        thread->ClearCurrentSession();
        if(session->IsDisconnected() && request.ShouldReschedule()){
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

    uv_stop(thread->GetLoop());
    pthread_exit(0);
  }

  bool PeerSessionThread::Start(){
    return Thread::StartThread(&thread_, thread_name_.data(), &HandleThread, (uword) this);
  }

  void PeerSessionThread::OnWalk(uv_handle_t* handle, void* data){
    uv_close(handle, &OnClose);
  }

  void PeerSessionThread::OnClose(uv_handle_t* handle){
    LOG(INFO) << "on-close.";
  }

  void PeerSessionThread::OnDisconnect(uv_async_t* handle){
    PeerSessionThread* thread = (PeerSessionThread*)handle->data;
    if(!thread->Disconnect())
      LOG(WARNING) << "couldn't disconnect peer session thread.";
  }

  bool PeerSessionThread::Stop(){
    SetState(PeerSessionThread::kStopping);
    if(HasSession()){
      if(!GetCurrentSession()->Disconnect()){
        LOG(WARNING) << "cannot disconnect current session.";
        return false;
      }
    }

    int err;
    void** result = NULL;
    if((err = pthread_join(thread_, result)) != 0){
      LOG(WARNING) << "couldn't join thread: " << strerror(err);
      return false;
    }
    return true;
  }
}

#endif//TOKEN_ENABLE_SERVER