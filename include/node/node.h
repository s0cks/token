#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include "node_info.h"

namespace Token{
    class Message;
    class PeerSession;
    class HandleMessageTask;
    class Node{
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

        friend class BlockMiner;
    public:
        ~Node(){}

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

        static NodeInfo GetInfo();
        static void Start();
        static void RegisterPeer(const std::string& node_id, PeerSession* peer);
        static void UnregisterPeer(const std::string& node_id);
        static void GetPeers(std::vector<PeerInfo>& peers);
        static uint32_t GetNumberOfPeers();
        static bool HasPeer(const std::string& node_id);
        static bool HasPeer(const NodeAddress& address);
        static bool ConnectTo(const NodeAddress& address);
        static bool Broadcast(Message* msg);
        static bool WaitForShutdown();
        static bool WaitForRunning();
        static bool Shutdown();
        static State GetState();

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

#endif //TOKEN_NODE_H