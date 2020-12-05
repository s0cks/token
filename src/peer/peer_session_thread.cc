#include "peer/peer_session_thread.h"
#include "peer/peer_session_manager.h"

namespace Token{
    void* PeerSessionThread::HandleThread(void* data){
        PeerSessionThread* thread = (PeerSessionThread*)data;
        thread->SetState(State::kStarting);

        char truncated_name[16];
        snprintf(truncated_name, 15, "PeerSession-%" PRId32, thread->GetWorkerID());
        pthread_setname_np(pthread_self(), truncated_name);

        // start-up logic here
        thread->SetState(State::kIdle);
        while(!thread->IsStopping()){
            PeerSessionManager::ConnectRequest request;
            if(PeerSessionManager::GetNextRequest(request, PeerSessionThread::kRequestTimeoutIntervalMilliseconds)){
                NodeAddress paddr = request.GetAddress();
                LOG(INFO) << "connecting to peer: " << paddr;
                std::shared_ptr<PeerSession> session = thread->CreateNewSession(paddr);
                thread->SetState(State::kConnected); //TODO: fix this is not "connected" it's connecting...
                if(!session->Connect() && request.ShouldReschedule()){
                    int32_t backoffs = request.GetNumberOfAttempts() * PeerSessionManager::kRetryBackoffSeconds;
                    LOG(WARNING) << "couldn't connect to peer " << paddr << ", rescheduling (" << backoffs << "s)...";
                    sleep(backoffs);
                    if(!PeerSessionManager::RescheduleRequest(request))
                        LOG(WARNING) << "couldn't reschedule peer: " << paddr;
                    continue;
                }

                if(session->IsError() && request.ShouldReschedule()){
                    int32_t backoffs = request.GetNumberOfAttempts() * PeerSessionManager::kRetryBackoffSeconds;
                    LOG(WARNING) << "couldn't connect to peer " << paddr << ", rescheduling (" << backoffs << "s)...";
                    sleep(backoffs);
                    if(!PeerSessionManager::RescheduleRequest(request))
                        LOG(WARNING) << "couldn't reschedule peer: " << paddr;
                    continue;
                }

                LOG(INFO) << "disconnected from peer: " << paddr;
            }
        }
        pthread_exit(0);
    }

    bool PeerSessionThread::Start(){
        int result;
        pthread_attr_t thread_attr;
        if((result = pthread_attr_init(&thread_attr)) != 0){
            LOG(WARNING) << "couldn't initialize the thread attributes: " << strerror(result);
            return false;
        }

        if((result = pthread_create(&thread_, &thread_attr, &HandleThread, this)) != 0){
            LOG(WARNING) << "couldn't start the session thread: " << strerror(result);
            return false;
        }

        if((result = pthread_attr_destroy(&thread_attr)) != 0){
            LOG(WARNING) << "couldn't destroy the thread attributes: " << strerror(result);
            return false;
        }
        return true;
    }

    bool PeerSessionThread::Stop(){
        SetState(State::kStopping);

        void** result = NULL;
        int err;
        if((err = pthread_join(thread_, result)) != 0){
            LOG(WARNING) << "couldn't join thread: " << strerror(err);
            return false;
        }
        return true;
    }
}