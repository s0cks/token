#ifdef TOKEN_ENABLE_SERVER

#include "peer/peer_session_thread.h"
#include "peer/peer_session_manager.h"

namespace Token{
  static std::mutex mutex_;
  static std::condition_variable cond_;
  static PeerSessionThread** threads_;

  static std::mutex requests_mutex_;
  static std::condition_variable requests_cond_;
  static std::queue<PeerSessionManager::ConnectRequest> requests_;

#define LOCK_GUARD std::lock_guard<std::mutex> __guard__(mutex_)
#define LOCK std::unique_lock<std::mutex> __lock__(mutex_)
#define WAIT cond_.wait(__lock__)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

  bool PeerSessionManager::Initialize(){
    LOCK_GUARD;
    threads_ = (PeerSessionThread**)malloc(sizeof(PeerSessionThread*)*FLAGS_num_peers);
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      threads_[idx] = new PeerSessionThread(idx);
      if(!threads_[idx]->Start()){
        LOG(WARNING) << "couldn't start peer session #" << idx;
        return false;
      }
    }

    PeerList peers;
    if(!BlockChainConfiguration::GetPeerList(peers)){
      LOG(WARNING) << "couldn't get list of peers from the configuration.";
      return true;
    }

    for(auto& it : peers)
      ScheduleRequest(it);
    return true;
  }

  bool PeerSessionManager::Shutdown(){
    LOCK_GUARD;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning()){
        std::shared_ptr<PeerSession> session = thread->GetCurrentSession();
        session->Disconnect();
        session->WaitForState(PeerSession::kDisconnected);
      }

#ifdef TOKEN_DEBUG
      LOG(INFO) << "stopping peer session thread #" << idx << "....";
#endif//TOKEN_DEBUG
      thread->Stop();
    }
    return true;
  }

  std::shared_ptr<PeerSession> PeerSessionManager::GetSession(const NodeAddress& address){
    LOCK_GUARD;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning()){
        std::shared_ptr<PeerSession> session = thread->GetCurrentSession();
        if(session->GetAddress() == address)
          return session;
      }
    }
    return nullptr;
  }

  std::shared_ptr<PeerSession> PeerSessionManager::GetSession(const UUID& uuid){
    LOCK_GUARD;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning()){
        std::shared_ptr<PeerSession> session = thread->GetCurrentSession();
        if(session->GetID() == uuid)
          return session;
      }
    }
    return nullptr;
  }

  bool PeerSessionManager::GetConnectedPeers(std::set<UUID>& peers){
    LOCK_GUARD;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread && thread->HasSession())
        peers.insert(thread->GetCurrentSession()->GetID());
    }
    return true;
  }

  int32_t PeerSessionManager::GetNumberOfConnectedPeers(){
    int32_t count = 0;
    LOCK_GUARD;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning())
        count++;
    }
    return count;
  }

  bool PeerSessionManager::ConnectTo(const NodeAddress& address){
    if(IsConnectedTo(address))
      return false;
    return ScheduleRequest(address);
  }

  bool PeerSessionManager::IsConnectedTo(const UUID& uuid){
    LOCK_GUARD;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning()){
        std::shared_ptr<PeerSession> session = thread->GetCurrentSession();
        if(session->GetID() == uuid)
          return true;
      }
    }
    return false;
  }

  bool PeerSessionManager::IsConnectedTo(const NodeAddress& address){
    LOCK_GUARD;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning()){
        std::shared_ptr<PeerSession> session = thread->GetCurrentSession();
        if(session->GetAddress() == address)
          return true;
      }
    }
    return false;
  }

  void PeerSessionManager::BroadcastPrepare(){
    LOCK_GUARD;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning())
        thread->GetCurrentSession()->SendPrepare();
    }
  }

  void PeerSessionManager::BroadcastCommit(){
    LOCK_GUARD;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning())
        thread->GetCurrentSession()->SendCommit();
    }
  }

  void PeerSessionManager::BroadcastDiscoveredBlock(){
    LOCK_GUARD;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning())
        thread->GetCurrentSession()->SendDiscoveredBlockHash();
    }
  }

  bool PeerSessionManager::ScheduleRequest(const NodeAddress& next, int16_t attempts){
    std::unique_lock<std::mutex> lock(requests_mutex_);
    if((int32_t) requests_.size() == FLAGS_num_peers)
      return false;
    requests_.push(ConnectRequest(next, attempts));
    requests_cond_.notify_one();
    return true;
  }

  bool PeerSessionManager::RescheduleRequest(const ConnectRequest& request){
    std::unique_lock<std::mutex> lock(requests_mutex_);
    if((int32_t) requests_.size() == FLAGS_num_peers)
      return false;
    requests_.push(ConnectRequest(request, true));
    requests_cond_.notify_one();
    return true;
  }

  bool PeerSessionManager::GetNextRequest(ConnectRequest& request, int64_t timeoutms){
    std::unique_lock<std::mutex> lock(requests_mutex_);
    requests_cond_.wait_for(lock, std::chrono::milliseconds(timeoutms));
    if(requests_.empty())
      return false;
    request = requests_.front();
    requests_.pop();
    return true;
  }
}

#endif//TOKEN_ENABLE_SERVER