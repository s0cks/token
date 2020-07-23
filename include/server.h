#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include "node_info.h"
#include "object.h"

namespace Token{
    class Message;
    class PeerSession;
    class HandleMessageTask;
    class Server{
    public:
        enum State{
            kStarting,
            kRunning,
            kSynchronizing,
            kStopping,
            kStopped
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
                case kStarting:
                    stream << "Starting";
                    break;
                case kRunning:
                    stream << "Running";
                    break;
                case kSynchronizing:
                    stream << "Synchronizing";
                    break;
                case kStopping:
                    stream << "Stopping";
                    break;
                case kStopped:
                    stream << "Stopped";
                    break;
                default:
                    stream << "Unknown";
                    break;
            }
            return stream;
        }
    private:
        static void LoadNodeInformation();
        static void SavePeers();
        static void LoadPeers();

        static void WaitForState(State state);
        static void SetState(State state);

        static uv_tcp_t* GetHandle();
        static void* NodeThread(void* ptr);
        static void HandleTerminateCallback(uv_async_t* handle);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

#define DECLARE_TASK(Name) \
    static void Handle##Name##Task(uv_work_t* handle); \
    static void After##Name##Task(uv_work_t* handle, int status);
    DECLARE_TASK(ResolveInventory);
    DECLARE_TASK(ResolveInventoryData);
#undef DECLARE_TASK

        static bool BroadcastMessage(const Handle<Message>& msg);

        friend class BlockMiner;
    public:
        ~Server(){}

        static inline bool
        ConnectTo(const std::string& address, uint32_t port){
            return ConnectTo(NodeAddress(address, port));
        }

        static inline NodeAddress
        GetAddress(){
            return GetInfo().GetNodeAddress();
        }

        static inline std::string
        GetID(){
            return GetInfo().GetNodeID();
        }

        static void Start();
        static void RegisterPeer(const std::string& node_id, PeerSession* peer);
        static void UnregisterPeer(const std::string& node_id);
        static void GetPeers(std::vector<PeerInfo>& peers);
        static bool HasPeer(const std::string& node_id);
        static bool HasPeer(const NodeAddress& address);
        static bool ConnectTo(const NodeAddress& address);
        static bool WaitForShutdown();
        static bool WaitForRunning();
        static bool Shutdown();
        static uint32_t GetNumberOfPeers();
        static State GetState();
        static NodeInfo GetInfo();

        template<typename T>
        static inline void
        Broadcast(const Handle<T>& msg){
            Handle<Message> m = msg.template CastTo<Message>();
            BroadcastMessage(m);
        }

        static inline bool
        IsStarting(){
            return GetState() == kStarting;
        }

        static inline bool
        IsRunning(){
            return GetState() == kRunning;
        }

        static inline bool
        IsSynchronizing(){
            return GetState() == kSynchronizing;
        }

        static inline bool
        IsStopping(){
            return GetState() == kStopping;
        }

        static inline bool
        IsStopped(){
            return GetState() == kStopped;
        }
    };
}

#endif //TOKEN_SERVER_H