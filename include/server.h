#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include <sstream>
#include <uv.h>

#include "peer.h"
#include "address.h"
#include "message.h"
#include "vthread.h"

namespace Token{
    class Message;
    class PeerSession;
    class HandleMessageTask;
    class Server : public Thread{
        friend class PeerSession;
    public:
        enum State{
            kStarting,
            kRunning,
            kSynchronizing,
            kStopping,
            kStopped,
            kError
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
        Server() = delete;

        static uv_tcp_t* GetHandle();
        static bool Initialize();
        static bool RegisterPeer(PeerSession* session);
        static bool UnregisterPeer(PeerSession* session);

        static void SetState(State state);
        static void HandleThread(uword parameter);
        static void HandleTerminateCallback(uv_async_t* handle);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    public:
        ~Server() = delete;

        static State GetState();
        static UUID GetID();
        static void WaitForState(State state);
        static bool Broadcast(const Handle<Message>& msg);
        static bool HasPeer(const UUID& uuid);
        static bool ConnectTo(const NodeAddress& address);
        static bool IsConnectedTo(const NodeAddress& address);
        static bool GetPeers(std::vector<UUID>& peers);
        static int GetNumberOfPeers();
        static PeerSession* GetPeer(const UUID& uuid);

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

        static bool Start(){
            if(!IsStopped())
                return false;
            if(!Initialize()){
                LOG(ERROR) << "couldn't initialize the server.";
                return false;
            }
            return Thread::Start("ServerThread", &HandleThread, 0) == 0;
        }

#define DECLARE_BROADCAST(Name) \
        static void Broadcast(const Handle<Name##Message>& msg){ Broadcast(msg.CastTo<Message>()); }
        FOR_EACH_MESSAGE_TYPE(DECLARE_BROADCAST)
#undef DECLARE_BROADCAST
    };

    class ServerSession : public Session{
        friend class Server;
    private:
        uv_tcp_t handle_;

        ServerSession():
            Session(&handle_),
            handle_(){
            handle_.data = this;
        }
    protected:
        virtual uv_stream_t* GetStream(){
            return (uv_stream_t*)&handle_;
        }

#define DECLARE_MESSAGE_HANDLER(Name) \
        virtual void Handle##Name##Message(const Handle<HandleMessageTask>& task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER
    public:
        ~ServerSession() = default;
    };
}

#endif //TOKEN_SERVER_H