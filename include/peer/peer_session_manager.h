#ifndef TOKEN_PEER_SESSION_MANAGER_H
#define TOKEN_PEER_SESSION_MANAGER_H

#ifdef TOKEN_ENABLE_SERVER

#include "peer/peer_session.h"

namespace Token{
#define FOR_EACH_PEER_SESSION_MANAGER_STATE(V) \
  V(Uninitialized)                             \
  V(Initializing)                              \
  V(Initialized)

#define FOR_EACH_PEER_SESSION_MANAGER_STATUS(V) \
  V(Ok)                                         \
  V(Warning)                                    \
  V(Error)

  class PeerSessionManager{
    //TODO:
    // - saving peers
    // - monitoring of peers
    // - heartbeats for the peers
    // - leverage state-machine
    friend class PeerSessionThread;
   public:
    static const int16_t kMaxNumberOfAttempts = 3;
    static const int32_t kRetryBackoffSeconds = 15;

    class ConnectRequest{
      friend class PeerSessionManager;
     private:
      NodeAddress address_;
      int16_t attempts_;

      ConnectRequest(const NodeAddress& addr, int16_t attempts):
        address_(addr),
        attempts_(attempts){}
      ConnectRequest(const ConnectRequest& request, bool is_retry):
        address_(request.address_),
        attempts_(is_retry ? request.GetNumberOfAttempts() + 1 : 0){}
     public:
      ConnectRequest():
        address_(),
        attempts_(0){}
      ConnectRequest(const ConnectRequest& other):
        address_(other.address_),
        attempts_(other.attempts_){}
      ~ConnectRequest() = default;

      NodeAddress GetAddress() const{
        return address_;
      }

      int16_t GetNumberOfAttempts() const{
        return attempts_;
      }

      bool ShouldReschedule() const{
        return GetNumberOfAttempts() < PeerSessionManager::kMaxNumberOfAttempts;
      }

      void operator=(const ConnectRequest& request){
        address_ = request.address_;
        attempts_ = request.attempts_;
      }

      friend bool operator==(const ConnectRequest& a, const ConnectRequest& b){
        return a.address_ == b.address_
               && a.attempts_ == b.attempts_;
      }

      friend bool operator!=(const ConnectRequest& a, const ConnectRequest& b){
        return !operator==(a, b);
      }

      friend bool operator<(const ConnectRequest& a, const ConnectRequest& b){
        if(a.address_ == b.address_){
          return a.attempts_ < b.attempts_;
        }
        return a.address_ < b.address_;
      }
    };

    enum State{
#define DEFINE_STATE(Name) k##Name,
      FOR_EACH_PEER_SESSION_MANAGER_STATE(DEFINE_STATE)
#undef DEFINE_STATE
    };

    friend std::ostream& operator<<(std::ostream& stream, const State& state){
      switch(state){
#define DEFINE_TOSTRING(Name) \
        case State::k##Name:  \
          return stream << #Name;
        FOR_EACH_PEER_SESSION_MANAGER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

    enum Status{
#define DEFINE_STATUS(Name) k##Name,
      FOR_EACH_PEER_SESSION_MANAGER_STATUS(DEFINE_STATUS)
#undef DEFINE_STATUS
    };

    friend std::ostream& operator<<(std::ostream& stream, const Status& status){
      switch(status){
#define DEFINE_TOSTRING(Name) \
        case Status::k##Name:   \
          return stream << #Name;
        FOR_EACH_PEER_SESSION_MANAGER_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }
   private:
    PeerSessionManager() = delete;

    static void SetState(const State& state);
    static void SetStatus(const Status& status);
    static bool ScheduleRequest(const NodeAddress& address, int16_t attempts = 1);
    static bool RescheduleRequest(const ConnectRequest& request);
    static bool GetNextRequest(ConnectRequest& request, int64_t timeoutms);
   public:
    ~PeerSessionManager() = delete;

    static State GetState();
    static Status GetStatus();
    static bool Initialize();
    static bool Shutdown();
    static bool ConnectTo(const NodeAddress& address);
    static bool IsConnectedTo(const UUID& uuid);
    static bool IsConnectedTo(const NodeAddress& address);
    static bool GetConnectedPeers(std::set<UUID>& peers);
    static int32_t GetNumberOfConnectedPeers();
    static PeerSession* GetSession(const UUID& uuid);
    static PeerSession* GetSession(const NodeAddress& address);
    static void BroadcastPrepare();
    static void BroadcastCommit();
    static void BroadcastDiscoveredBlock();

    static inline bool
    HasConnectedPeers(){
      return GetNumberOfConnectedPeers() > 0;
    }
  };
}

#endif//TOKEN_ENABLE_SERVER
#endif //TOKEN_PEER_SESSION_MANAGER_H