#ifndef TOKEN_SERVER_H
#define TOKEN_SERVER_H

#include <uv.h>
#include <sstream>
#include "address.h"
#include "message.h"
#include "vthread.h"
#include "session.h"

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

    class PeerSession;
    class HandleMessageTask;
    class Server : public Thread{
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
    private:
        Server() = delete;

        static uv_tcp_t* GetHandle();
        static void SetStatus(Status status);
        static void SetState(State state);
        static void HandleThread(uword parameter);
        static void HandleTerminateCallback(uv_async_t* handle);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
    public:
        ~Server() = delete;

        static State GetState();
        static Status GetStatus();
        static std::string GetStatusMessage();
        static UUID GetID();
        static void WaitForState(State state);
        static bool Initialize();
        static bool Shutdown();

#define DEFINE_STATE_CHECK(Name) \
        static inline bool Is##Name(){ return GetState() == Server::k##Name; }
        FOR_EACH_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

#define DEFINE_STATUS_CHECK(Name) \
        static inline bool Is##Name(){ return GetStatus() == Server::k##Name; }
        FOR_EACH_SERVER_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK
    };

    class ServerSession : public Session{
        friend class Server;
    private:
        UUID uuid_;

        ServerSession(uv_loop_t* loop):
            Session(loop),
            uuid_(){}

        void SetID(const UUID& uuid){
            uuid_ = uuid;
        }

#define DECLARE_MESSAGE_HANDLER(Name) \
        static void Handle##Name##Message(HandleMessageTask* task);
        FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_HANDLER)
#undef DECLARE_MESSAGE_HANDLER
    public:
        UUID GetID() const{
            return uuid_;
        }

        std::string ToString() const{
            std::stringstream ss;
            ss << "ServerSession(" << GetID() << ")";
            return ss.str();
        }

        static ServerSession* NewInstance(uv_loop_t* loop){
            return new ServerSession(loop);
        }
    };
}

#endif //TOKEN_SERVER_H