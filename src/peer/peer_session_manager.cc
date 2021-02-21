#ifdef TOKEN_ENABLE_SERVER

#include "configuration.h"
#include "peer/peer.h"
#include "peer/peer_session_thread.h"
#include "peer/peer_session_manager.h"

namespace token{
  static std::default_random_engine engine;
  static RelaxedAtomic<PeerSessionManager::State> state_ = { PeerSessionManager::kUninitializedState };
  static PeerSessionThread** threads_;
  static ConnectionRequestQueue queue_(TOKEN_CONNECTION_QUEUE_SIZE);
  static std::map<ThreadId, ConnectionRequestQueue*> queues_;

  PeerSessionManager::State PeerSessionManager::GetState(){
    return state_;
  }

  void PeerSessionManager::SetState(const State& state){
    state_ = state;
  }

  bool PeerSessionManager::Initialize(){
    if(!IsUninitializedState()){
#ifdef TOKEN_DEBUG
      LOG(WARNING) << "cannot re-initialize the peer session manager.";
#endif//TOKEN_DEBUG
      return false;
    }

    RegisterQueue(pthread_self(), &queue_);//TODO: cross-platform fix
    threads_ = (PeerSessionThread**) malloc(sizeof(PeerSessionThread*) * FLAGS_num_peers);
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx] = new PeerSessionThread(idx);
      if(!threads_[idx]->Start()){
        LOG(WARNING) << "couldn't start peer session #" << idx;
        return false;
      }
      RegisterQueue(thread->GetThreadId(), thread->GetQueue());
    }

    PeerList peers;
    if(!ConfigurationManager::GetPeerList(TOKEN_CONFIGURATION_NODE_PEERS, peers)){
      LOG(WARNING) << "couldn't get list of peers from the configuration.";
      return true;
    }

    for(auto& it : peers)
      ScheduleRequest(it);
    return true;
  }

  bool PeerSessionManager::Shutdown(){
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(!thread->Shutdown())
        return false;
    }
    return true;
  }

  bool PeerSessionManager::WaitForShutdown(){
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(!thread->WaitForShutdown())
        return false;
    }
    return true;
  }

  PeerSession* PeerSessionManager::GetSession(const NodeAddress& address){
    //TODO: implement
    LOG(ERROR) << "not implemented yet.";
    return nullptr;
  }

  PeerSession* PeerSessionManager::GetSession(const UUID& uuid){
    //TODO: implement
    LOG(ERROR) << "not implemented yet.";
    return nullptr;
  }

  bool PeerSessionManager::GetConnectedPeers(std::set<UUID>& peers){
    //TODO: implement
    LOG(ERROR) << "not implemented yet.";
    return false;
  }

  int32_t PeerSessionManager::GetNumberOfConnectedPeers(){
    int32_t count = 0;
    for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx];
      if(thread->IsConnected())
        count++;
    }
    return count;
  }

  bool PeerSessionManager::ConnectTo(const NodeAddress& address){
    if(IsConnectedTo(address))
      return false;
    ScheduleRequest(address);
    return true;
  }

  bool PeerSessionManager::IsConnectedTo(const UUID& uuid){
    //TODO: implement
    LOG(ERROR) << "not implemented yet.";
    return false;
  }

  bool PeerSessionManager::IsConnectedTo(const NodeAddress& address){
    //TODO: implement
    LOG(ERROR) << "not implemented yet.";
    return false;
  }

#define DEFINE_PAXOS_BROADCAST(Name) \
  void PeerSessionManager::Broadcast##Name(){}//TODO: implement

  DEFINE_PAXOS_BROADCAST(Prepare);
  DEFINE_PAXOS_BROADCAST(Promise);
  DEFINE_PAXOS_BROADCAST(Commit);
  DEFINE_PAXOS_BROADCAST(DiscoveredBlock);

  void PeerSessionManager::RegisterQueue(const ThreadId& thread, ConnectionRequestQueue* queue){
    auto pos = queues_.find(thread);
    if(pos != queues_.end()){
      LOG(WARNING) << "cannot re-register queue for " << GetThreadName(thread) << " thread.";
      return;
    }
    if(!queues_.insert({ thread, queue }).second)
      LOG(WARNING) << "cannot register queue for " << GetThreadName(thread) << " thread.";
  }

  ConnectionRequestQueue* PeerSessionManager::GetQueue(const ThreadId& thread){
    for(auto& it : queues_){
      if(pthread_equal(it.first, thread))
        return it.second;
    }
    LOG(INFO) << "cannot find queue for: " << thread;
    return nullptr;
  }

  //TODO: optimize
  ConnectionRequestQueue* PeerSessionManager::GetRandomQueue(){
    std::vector<ThreadId> keys;
    for(auto& it : queues_)
      keys.push_back(it.first);

    std::uniform_int_distribution<int> distribution(0, keys.size() - 1);

    auto pos = queues_.find(keys[distribution(engine)]);
    if(pos == queues_.end())
      return nullptr;
    return pos->second;
  }

  void PeerSessionManager::ScheduleRequest(const NodeAddress& address, const ConnectionAttemptCounter attempts){
#ifdef TOKEN_DEBUG
    LOG(INFO) << "scheduling connection request for " << address << " (" << attempts << " attempts)....";
#endif//TOKEN_DEBUG

    ConnectionRequest* request = new ConnectionRequest(address, attempts); //TODO: memory-leak
    if(!queue_.Push(request))
      LOG(WARNING) << "cannot schedule connection request for " << address;
  }
}

#endif//TOKEN_ENABLE_SERVER