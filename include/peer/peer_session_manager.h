#ifndef TOKEN_PEER_SESSION_MANAGER_H
#define TOKEN_PEER_SESSION_MANAGER_H

#ifdef TOKEN_ENABLE_SERVER

#include "peer/peer_queue.h"
#include "peer/peer_session.h"

namespace token{


#define FOR_EACH_PEER_SESSION_MANAGER_STATE(V) \
  V(Uninitialized)                             \
  V(Initializing)                              \
  V(Initialized)

  class PeerSessionManager{
    //TODO:
    // - saving peers
    // - monitoring of peers
    // - heartbeats for the peers
    // - leverage state-machine
    friend class PeerSessionThread;
   public:
    static const int32_t kRetryBackoffSeconds = 15;

    enum State{
#define DEFINE_STATE(Name) k##Name##State,
      FOR_EACH_PEER_SESSION_MANAGER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name##State:  \
          return stream << #Name;
        FOR_EACH_PEER_SESSION_MANAGER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   private:
    PeerSessionManager() = delete;

    static void SetState(const State& state);
    static void RegisterQueue(const ThreadId& thread, ConnectionRequestQueue* queue);
    static void ScheduleRequest(const NodeAddress& address, const ConnectionAttemptCounter attempts=TOKEN_MAX_CONNECTION_ATTEMPTS);
    static ConnectionRequestQueue* GetRandomQueue();
    static ConnectionRequestQueue* GetQueue(const ThreadId& thread);

    static inline ConnectionRequestQueue*
    GetThreadQueue(){
      return GetQueue(pthread_self());
    }
   public:
    ~PeerSessionManager() = delete;

    static State GetState();
    static bool Initialize();

    static bool Shutdown();
    static bool WaitForShutdown();

    static bool ConnectTo(const NodeAddress& address);
    static bool IsConnectedTo(const UUID& uuid);
    static bool IsConnectedTo(const NodeAddress& address);
    static bool GetConnectedPeers(std::set<UUID>& peers);
    static int32_t GetNumberOfConnectedPeers();
    static PeerSession* GetSession(const UUID& uuid);
    static PeerSession* GetSession(const NodeAddress& address);
    static void BroadcastPrepare();
    static void BroadcastPromise();
    static void BroadcastCommit();
    static void BroadcastDiscoveredBlock();

    static inline bool
    HasConnectedPeers(){
      return GetNumberOfConnectedPeers() > 0;
    }

#define DEFINE_CHECK(Name) \
    static inline bool Is##Name##State(){ return GetState() == State::k##Name##State; }
    FOR_EACH_PEER_SESSION_MANAGER_STATE(DEFINE_CHECK)
#undef DEFINE_CHECK
  };
}

#endif//TOKEN_ENABLE_SERVER
#endif //TOKEN_PEER_SESSION_MANAGER_H