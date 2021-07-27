#include "json.h"
#include "flags.h"
#include "configuration.h"

#include "peer/peer.h"
#include "peer/peer_session_thread.h"
#include "peer/peer_session_manager.h"

namespace token{
  static std::default_random_engine engine;
  static atomic::RelaxedAtomic<PeerSessionManager::State> state_(PeerSessionManager::State::kUninitializedState);
  static PeerSessionThread** threads_;
  static ConnectionRequestQueue queue_(TOKEN_CONNECTION_QUEUE_SIZE);
  static std::map<ThreadId, ConnectionRequestQueue*> queues_;

  PeerSessionManager::State PeerSessionManager::GetState(){
    return (State)state_;
  }

  void PeerSessionManager::SetState(const State& state){
    state_ = state;
  }

  static inline void
  GetPeerList(PeerList& peers){
    //TODO:
    // - better error handling
    // - fetch peer list from config

    utils::AddressResolver resolver;
    if(!FLAGS_remote.empty()){
      utils::AddressList addresses;
      if(!resolver.ResolveAddresses(FLAGS_remote, addresses)){
        DLOG(WARNING) << "couldn't resolve peer: " << FLAGS_remote;
        return;
      }
      peers.insert(addresses.begin(), addresses.end());
    }
  }

  bool PeerSessionManager::Initialize(){
    if (!IsUninitializedState()){
      DLOG(WARNING) << "cannot re-initialize the PeerSessionManager.";
      return false;
    }

    RegisterQueue(pthread_self(), &queue_);//TODO: cross-platform fix
    threads_ = (PeerSessionThread**) malloc(sizeof(PeerSessionThread*) * FLAGS_num_peers);
    for(auto idx = 0; idx < FLAGS_num_peers; idx++){
      PeerSessionThread* thread = threads_[idx] = new PeerSessionThread(idx);
      DLOG(INFO) << "starting PeerSessionThread #" << idx << "....";
      if(!threads_[idx]->Start()){
        LOG(WARNING) << "couldn't start peer session #" << idx;
        return false;
      }
      RegisterQueue(thread->GetThreadId(), thread->GetQueue());
    }

    PeerList peers;
    GetPeerList(peers);//TODO: error handling

    DLOG(INFO) << "connecting to " << peers.size() << " peers.....";
    for (auto& it : peers)
      ScheduleRequest(it);
    return true;
  }

#define FOR_EACH_WORKER(Name) \
  for(int32_t idx = 0; idx < FLAGS_num_peers; idx++){ \
    auto Name = threads_[idx];
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

  bool PeerSessionManager::ConnectTo(const utils::Address& address){
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

  bool PeerSessionManager::IsConnectedTo(const utils::Address& address){
    FOR_EACH_WORKER(thread)
      if (thread->IsConnected() && thread->GetPeerAddress() == address)
        return true;
    END_FOR_EACH_WORKER
    return false;
  }

#define DEFINE_PAXOS_BROADCAST(Name) \
  void PeerSessionManager::Broadcast##Name(){ \
    DLOG(INFO) << "broadcasting " << #Name << " to " << GetNumberOfConnectedPeers() << " peers...."; \
    FOR_EACH_WORKER(thread)          \
      if(thread->IsConnected()){     \
        thread->Send##Name();       \
      }                              \
    END_FOR_EACH_WORKER              \
  }

  DEFINE_PAXOS_BROADCAST(Prepare);
//  DEFINE_PAXOS_BROADCAST(Promise);
  DEFINE_PAXOS_BROADCAST(Commit);
//  DEFINE_PAXOS_BROADCAST(DiscoveredBlock);
//  DEFINE_PAXOS_BROADCAST(Accepted);
//  DEFINE_PAXOS_BROADCAST(Rejected);
#undef DEFINE_PAXOS_BROADCAST

  void PeerSessionManager::RegisterQueue(const ThreadId& thread, ConnectionRequestQueue* queue){
    auto pos = queues_.find(thread);
    if(pos != queues_.end()){
      LOG(WARNING) << "cannot re-register queue for " << platform::GetThreadName(thread) << " thread.";
      return;
    }
    if(!queues_.insert({ thread, queue }).second)
      LOG(WARNING) << "cannot register queue for " << platform::GetThreadName(thread) << " thread.";
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

  void PeerSessionManager::ScheduleRequest(const utils::Address& address, const ConnectionAttemptCounter& attempts){
    DLOG(INFO) << "scheduling connection request for " << address << " (" << attempts << " attempts)....";
    auto request = new ConnectionRequest(address, attempts); //TODO: memory-leak
    if(!queue_.Push(request))
      LOG(WARNING) << "cannot schedule connection request for " << address;
  }

  bool PeerSessionManager::GetConnectedPeers(json::Writer& writer){
    return false;
  }
}