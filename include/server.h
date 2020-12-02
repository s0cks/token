#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include <sstream>
#include <uv.h>

#include "peer_session.h"
#include "address.h"
#include "message.h"
#include "vthread.h"

namespace Token{
#define FOR_EACH_SERVER_STATE(V) \
    V(Starting) \
    V(Running) \
    V(Synchronizing) \
    V(Stopping) \
    V(Stopped)

#define FOR_EACH_SERVER_STATUS(V) \
    V(Ok)                         \
    V(Warning)                    \
    V(Error)

    class Message;
    class PeerSession;
    class HandleMessageTask;
    class Server : public Thread{
        friend class PeerSession;
        friend class Scavenger;
    public:
        enum State{
#define DEFINE_SERVER_STATE(Name) k##Name,
            FOR_EACH_SERVER_STATE(DEFINE_SERVER_STATE)
#undef DEFINE_SERVER_STATE
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
#define DEFINE_TOSTRING(Name) \
                case Server::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_SERVER_STATE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
            }
        }

        enum Status{
#define DEFINE_SERVER_STATUS(Name) k##Name,
            FOR_EACH_SERVER_STATUS(DEFINE_SERVER_STATUS)
#undef DEFINE_SERVER_STATUS
        };

        friend std::ostream& operator<<(std::ostream& stream, const Status& status){
            switch(status){
#define DEFINE_TOSTRING(Name) \
                case Server::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_SERVER_STATUS(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
            }
        }

        static const size_t kMaxNumberOfPeers = 16;
    private:
        Server() = delete;

        static uv_tcp_t* GetHandle();
        static bool Accept(WeakObjectPointerVisitor* vis);
        static bool RegisterPeer(const Handle<PeerSession>& session);
        static bool UnregisterPeer(const Handle<PeerSession>& session);
        static bool SavePeerList();
        static void SetStatus(Status status);
        static void SetState(State state);
        static void HandleThread(uword parameter);
        static void HandleTerminateCallback(uv_async_t* handle);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    public:
        ~Server() = delete;

        static NodeAddress GetCallbackAddress();
        static State GetState();
        static Status GetStatus();
        static std::string GetStatusMessage();
        static UUID GetID();
        static void WaitForState(State state);
        static bool Initialize();
        static bool Shutdown();
        static bool Broadcast(const Handle<Message>& msg);
        static bool ConnectTo(const NodeAddress& address);
        static bool IsConnectedTo(const NodeAddress& address);
        static bool IsConnectedTo(const UUID& uuid);
        static bool GetPeers(PeerList& peers);
        static int GetNumberOfPeers();
        static Handle<PeerSession> GetPeer(const NodeAddress& address);
        static Handle<PeerSession> GetPeer(const UUID& uuid);

        static inline bool
        ConnectTo(const std::string& address, uint32_t port){
            return ConnectTo(NodeAddress(address, port));
        }

#define DEFINE_STATE_CHECK(Name) \
        static inline bool Is##Name(){ return GetState() == Server::k##Name; }
        FOR_EACH_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

#define DEFINE_STATUS_CHECK(Name) \
        static inline bool Is##Name(){ return GetStatus() == Server::k##Name; }
        FOR_EACH_SERVER_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK


#define DECLARE_BROADCAST(Name) \
        static void Broadcast(const Handle<Name##Message>& msg){ Broadcast(msg.CastTo<Message>()); }
        FOR_EACH_MESSAGE_TYPE(DECLARE_BROADCAST)
#undef DECLARE_BROADCAST
    };

    class ServerSession : public Session{
        friend class Server;
    private:
        ServerSession(uv_loop_t* loop):
            Session(loop){
            SetType(Type::kServerSessionType);
        }

#define DECLARE_MESSAGE_HANDLER(Name) \
        static void Handle##Name##Message(const Handle<HandleMessageTask>& task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER
    public:
        static Handle<ServerSession> NewInstance(uv_loop_t* loop){
            return new ServerSession(loop);
        }
    };
}

#endif //TOKEN_SERVER_H