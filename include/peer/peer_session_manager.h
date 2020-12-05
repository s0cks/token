#ifndef TOKEN_PEER_SESSION_MANAGER_H
#define TOKEN_PEER_SESSION_MANAGER_H

#include "peer/peer_session.h"

namespace Token{
    class PeerSessionManager{
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
                if(a.address_ == b.address_)
                    return a.attempts_ < b.attempts_;
                return a.address_ < b.address_;
            }
        };
    private:
        PeerSessionManager() = delete;

        static bool ScheduleRequest(const NodeAddress& address, int16_t attempts=1);
        static bool RescheduleRequest(const ConnectRequest& request);
        static bool GetNextRequest(ConnectRequest& request, int64_t timeoutms);
    public:
        ~PeerSessionManager() = delete;

        static bool Initialize();
        static bool Shutdown();
        static bool IsConnectedTo(const UUID& uuid);
        static bool IsConnectedTo(const NodeAddress& address);
        static bool ConnectTo(const NodeAddress& address);
        static int32_t GetNumberOfConnectedPeers();
        static std::shared_ptr<PeerSession> GetSession(const UUID& uuid);
        static std::shared_ptr<PeerSession> GetSession(const NodeAddress& address);
        static void BroadcastPrepare();
        static void BroadcastCommit();
    };
}

#endif //TOKEN_PEER_SESSION_MANAGER_H