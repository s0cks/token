#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include <sstream>
#include <uv.h>
#include "address.h"
#include "handle.h"

namespace Token{
    class Message;
    class PeerSession;
    class HandleMessageTask;
    class Server{
        friend class PeerSession;
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

        static const size_t kMaxNumberOfPeers = 16;
    private:
        static void LoadNodeInformation();
        static void SavePeers();
        static void LoadPeers();
        static void RegisterPeer(PeerSession* peer);
        static void UnregisterPeer(PeerSession* peer);
        static void SetState(State state);
        static uv_tcp_t* GetHandle();
        static void* NodeThread(void* ptr);
        static void HandleTerminateCallback(uv_async_t* handle);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    public:
        ~Server() = delete;

        static State GetState();
        static std::string GetID();
        static size_t GetNumberOfPeers();
        static bool Start();
        static bool Shutdown();
        static bool HasPeer(const NodeAddress& address);
        static bool HasPeer(const std::string& id);
        static bool ConnectTo(const NodeAddress& address);
        static bool Broadcast(const Handle<Message>& msg);
        static void WaitForState(State state);

        static inline bool
        ConnectTo(const std::string& address, uint32_t port){
            return ConnectTo(NodeAddress(address, port));
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