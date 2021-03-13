#ifdef TOKEN_ENABLE_SERVER

#include "configuration.h"
#include "peer/peer.h"
#include "peer/peer_session_thread.h"
#include "peer/peer_session_manager.h"

namespace token{
  static std::default_random_engine engine;
  static RelaxedAtomic<PeerSessionManager::State> state_ = {PeerSessionManager::kUninitializedState};
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
    if (!IsUninitializedState()){
#ifdef TOKEN_DEBUG
      LOG(WARNING) << "cannot re-initialize the peer session manager.";
#endif//TOKEN_DEBUG
      return false;
    }

    RegisterQueue(pthread_self(), &queue_);//TODO: cross-platform fix
    threads_ = (PeerSessionThread**) malloc(sizeof(PeerSessionThread*) * FLAGS_num_peers);
    for (int32_t idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx] = new PeerSessionThread(idx);
      if (!threads_[idx]->Start()){
        LOG(WARNING) << "couldn't start peer session #" << idx;
        return false;
      }
      RegisterQueue(thread->GetThreadId(), thread->GetQueue());
    }

    PeerList peers;
    if (!ConfigurationManager::GetInstance()->GetPeerList(TOKEN_CONFIGURATION_NODE_PEERS, peers)){
      LOG(WARNING) << "couldn't get list of peers from the configuration.";
      return true;
    }

    for (auto& it : peers)
      ScheduleRequest(it);
    return true;
  }

#define FOR_EACH_WORKER(Name) \
  for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){ \
    PeerSessionThread* Name = threads_[idx];
#define END_FOR_EACH_WORKER \
  }

  bool PeerSessionManager::Shutdown(){
    FOR_EACH_WORKER(thread)
      if (!thread->Shutdown())
        return false;
    END_FOR_EACH_WORKER
    return true;
  }

  bool PeerSessionManager::WaitForShutdown(){
    FOR_EACH_WORKER(thread)
      if (!thread->WaitForShutdown())
        return false;
    END_FOR_EACH_WORKER
    return true;
  }

  PeerSession* PeerSessionManager::GetSession(const NodeAddress& address){
    FOR_EACH_WORKER(thread)
      if (thread->IsConnected() && thread->GetPeerAddress() == address)
        return thread->GetPeerSession();
    END_FOR_EACH_WORKER
    return nullptr;
  }

  PeerSession* PeerSessionManager::GetSession(const UUID& uuid){
    FOR_EACH_WORKER(thread)
      if (thread->IsConnected() && thread->GetPeerID() == uuid)
        return thread->GetPeerSession();
    END_FOR_EACH_WORKER
    return nullptr;
  }

  bool PeerSessionManager::GetConnectedPeers(std::set<UUID>& peers){
    FOR_EACH_WORKER(thread)
      if (thread->IsConnected() && !peers.insert(thread->GetPeerID()).second)
        return false;
    END_FOR_EACH_WORKER
    return true;
  }

  int32_t PeerSessionManager::GetNumberOfConnectedPeers(){
    int32_t count = 0;
    FOR_EACH_WORKER(thread)
      if (thread->IsConnected())
        count++;
    END_FOR_EACH_WORKER
    return count;
  }

  bool PeerSessionManager::ConnectTo(const NodeAddress& address){
    if (IsConnectedTo(address))
      return false;
    ScheduleRequest(address);
    return true;
  }

  bool PeerSessionManager::IsConnectedTo(const UUID& uuid){
    FOR_EACH_WORKER(thread)
      if (thread->IsConnected() && thread->GetPeerID() == uuid)
        return true;
    END_FOR_EACH_WORKER
    return false;
  }

  bool PeerSessionManager::IsConnectedTo(const NodeAddress& address){
    FOR_EACH_WORKER(thread)
      if (thread->IsConnected() && thread->GetPeerAddress() == address)
        return true;
    END_FOR_EACH_WORKER
    return false;
  }

#define DEFINE_PAXOS_BROADCAST(Name) \
  void PeerSessionManager::Broadcast##Name(){ \
    FOR_EACH_WORKER(thread)          \
      if(thread->IsConnected()){     \
        PeerSession* session = thread->GetPeerSession(); \
        session->Send##Name();       \
      }                              \
    END_FOR_EACH_WORKER              \
  }

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

  static inline bool
  WritePeerInfo(Json::Writer& writer, PeerSession* session){
    return writer.StartObject()
        && Json::SetField(writer, "id", session->GetID())
        && Json::SetField(writer, "address", session->GetAddress())
        && writer.EndObject();
  }

  bool PeerSessionManager::GetConnectedPeers(Json::Writer& writer){
    FOR_EACH_WORKER(thread)
      if(thread->IsConnected() && !WritePeerInfo(writer, thread->GetPeerSession()))
        return false;
    END_FOR_EACH_WORKER
    return true;
  }
}

#endif//TOKEN_ENABLE_SERVER