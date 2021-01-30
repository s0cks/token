#ifdef TOKEN_ENABLE_SERVER

#include "peer/peer_session_thread.h"
#include "peer/peer_session_manager.h"

namespace Token{
  static std::mutex mutex_;
  static std::condition_variable cond_;
  static PeerSessionManager::State state_ = PeerSessionManager::kUninitialized;
  static PeerSessionManager::Status status_ = PeerSessionManager::kOk;
  static PeerSessionThread** threads_;
  static std::queue<PeerSessionManager::ConnectRequest> requests_;

#define LOCK_GUARD std::lock_guard<std::mutex> __guard__(mutex_)
#define LOCK std::unique_lock<std::mutex> __lock__(mutex_)
#define UNLOCK __lock__.unlock()
#define WAIT cond_.wait(__lock__)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

  PeerSessionManager::State PeerSessionManager::GetState(){
    std::lock_guard<std::mutex> guard(mutex_);
    return state_;
  }

  void PeerSessionManager::SetState(const State& state){
    LOCK;
    state_ = state;
    UNLOCK;
    SIGNAL_ALL;
  }

  PeerSessionManager::Status PeerSessionManager::GetStatus(){
    std::lock_guard<std::mutex> guard(mutex_);
    return status_;
  }

  void PeerSessionManager::SetStatus(const Status& status){
    LOCK;
    status_ = status;
    UNLOCK;
    SIGNAL_ALL;
  }

  bool PeerSessionManager::Initialize(){
    std::unique_lock<std::mutex> lock(mutex_);
    threads_ = (PeerSessionThread**) malloc(sizeof(PeerSessionThread*) * FLAGS_num_peers);
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      threads_[idx] = new PeerSessionThread(idx);
      if(!threads_[idx]->Start()){
        LOG(WARNING) << "couldn't start peer session #" << idx;
        return false;
      }
    }
    lock.unlock();

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
    std::lock_guard<std::mutex> guard(mutex_);
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning() && thread->HasSession()){
        PeerSession* session = thread->GetCurrentSession();
        session->Disconnect();
        session->WaitForState(Session::kDisconnectedState);
      }

      LOG(INFO) << "stopping peer session thread #" << idx << "....";
      thread->Stop();
    }
    return true;
  }

  PeerSession* PeerSessionManager::GetSession(const NodeAddress& address){
    std::lock_guard<std::mutex> guard(mutex_);
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning() && thread->HasSession()){
        PeerSession* session = thread->GetCurrentSession();
        if(session->GetAddress() == address){
          return session;
        }
      }
    }
    return nullptr;
  }

  PeerSession* PeerSessionManager::GetSession(const UUID& uuid){
    std::lock_guard<std::mutex> guard(mutex_);
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning() && thread->HasSession()){
        PeerSession* session = thread->GetCurrentSession();
        LOG(INFO) << "checking " << session->GetID() << " <=> " << uuid;
        if(session->GetID() == uuid)
          return session;
      }
    }
    return nullptr;
  }

  bool PeerSessionManager::GetConnectedPeers(std::set<UUID>& peers){
    std::lock_guard<std::mutex> guard(mutex_);
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread && thread->HasSession())
        peers.insert(thread->GetCurrentSession()->GetID());
    }
    return true;
  }

  int32_t PeerSessionManager::GetNumberOfConnectedPeers(){
    std::lock_guard<std::mutex> guard(mutex_);

    int32_t count = 0;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning() && thread->HasSession())
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
    std::lock_guard<std::mutex> guard(mutex_);
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning() && thread->HasSession()){
        PeerSession* session = thread->GetCurrentSession();
        if(session->GetID() == uuid){
          return true;
        }
      }
    }
    return false;
  }

  bool PeerSessionManager::IsConnectedTo(const NodeAddress& address){
    std::lock_guard<std::mutex> guard(mutex_);
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsRunning() && thread->HasSession()){
        PeerSession* session = thread->GetCurrentSession();
        if(session->GetAddress() == address){
          return true;
        }
      }
    }
    return false;
  }

#define DEFINE_PAXOS_BROADCAST(Name) \
  void PeerSessionManager::Broadcast##Name(){ \
    std::lock_guard<std::mutex> guard(mutex_);\
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){ \
      PeerSessionThread* thread = threads_[idx];        \
      if(thread->IsRunning() && thread->HasSession())   \
        thread->GetCurrentSession()->Send##Name();      \
    }                                \
  }

  DEFINE_PAXOS_BROADCAST(Prepare);
  DEFINE_PAXOS_BROADCAST(Promise);
  DEFINE_PAXOS_BROADCAST(Commit);
  DEFINE_PAXOS_BROADCAST(DiscoveredBlock);

  bool PeerSessionManager::ScheduleRequest(const NodeAddress& next, int16_t attempts){
    std::unique_lock<std::mutex> lock(mutex_);
    if((int32_t)requests_.size() == FLAGS_num_peers){
      lock.unlock();
      return false;
    }

    requests_.push(ConnectRequest(next, attempts));
    lock.unlock();
    cond_.notify_one();
    return true;
  }

  bool PeerSessionManager::RescheduleRequest(const ConnectRequest& request){
    std::unique_lock<std::mutex> lock(mutex_);
    if((int32_t) requests_.size() == FLAGS_num_peers){
      lock.unlock();
      return false;
    }

    lock.unlock();
    requests_.push(ConnectRequest(request, true));
    cond_.notify_one();
    return true;
  }

  bool PeerSessionManager::GetNextRequest(ConnectRequest& request, int64_t timeoutms){
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait_for(lock, std::chrono::milliseconds(timeoutms));
    if(requests_.empty()){
      lock.unlock();
      return false;
    }

    lock.unlock();
    request = requests_.front();
    requests_.pop();
    return true;
  }
}

#endif//TOKEN_ENABLE_SERVER