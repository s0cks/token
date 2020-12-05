#ifndef TOKEN_HEALTHCHECK_H
#define TOKEN_HEALTHCHECK_H

#include <uv.h>
#include <libconfig.h++>

#include "object.h"
#include "vthread.h"
#include "http/router.h"
#include "http/session.h"
#include "http/request.h"

namespace Token{
#define FOR_EACH_HEALTHCHECK_SERVER_STATE(V) \
    V(Starting)                 \
    V(Running)                  \
    V(Stopping)                 \
    V(Stopped)

#define FOR_EACH_HEALTHCHECK_SERVER_STATUS(V) \
    V(Ok)                                     \
    V(Warning)                                \
    V(Error)

#define FOR_EACH_HEALTHCHECK_SERVICE_ENDPOINT(V) \
    V(Get, Ready, "/ready")                           \
    V(Get, Live, "/live")                             \
    V(Get, PeerStatus, "/status/peers")               \
    V(Get, BlockChainStatus, "/status/chain")

    class HealthCheckService : public Thread{
        friend class Scavenger;
        friend class HttpSession;
    public:
        enum State{
#define DECLARE_STATE(Name) k##Name,
            FOR_EACH_HEALTHCHECK_SERVER_STATE(DECLARE_STATE)
#undef DECLARE_STATE
        };

        enum Status{
#define DECLARE_STATUS(Name) k##Name,
            FOR_EACH_HEALTHCHECK_SERVER_STATUS(DECLARE_STATUS)
#undef DECLARE_STATUS
        };

        friend std::ostream& operator<<(std::ostream& stream, const State& state){
            switch(state){
#define DECLARE_TOSTRING(Name) \
                case State::k##Name: \
                    stream << #Name; \
                    return stream;
                FOR_EACH_HEALTHCHECK_SERVER_STATE(DECLARE_TOSTRING)
#undef DECLARE_TOSTRING
            }
        }

        friend std::ostream& operator<<(std::ostream& stream, const Status& status){
            switch(status){
#define DECLARE_TOSTRING(Name) \
                case Status::k##Name: \
                    stream << #Name;  \
                    return stream;
                FOR_EACH_HEALTHCHECK_SERVER_STATUS(DECLARE_TOSTRING)
#undef DECLARE_TOSTRING
            }
        }

        static const int64_t kMaxNumberOfSessions = 64;
    private:
        HealthCheckService() = delete;
        static HttpRouter* GetRouter();
        static bool RegisterSession(HttpSession* session);
        static bool UnregisterSession(HttpSession* session);
        static void SetRouter(HttpRouter* router);
        static void SetState(State state);
        static void SetStatus(Status status);
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnShutdown(uv_shutdown_t* req, int status);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        static void HandleServiceThread(uword parameter);

        static void HandleUnknownEndpoint(HttpSession* session, HttpRequest* request);
#define DECLARE_ENDPOINT(Method, Name, Path) \
        static void Handle##Name##Endpoint(HttpSession* session, HttpRequest* request);
        FOR_EACH_HEALTHCHECK_SERVICE_ENDPOINT(DECLARE_ENDPOINT)
#undef DECLARE_ENDPOINT
    public:
        ~HealthCheckService() = delete;

        static State GetState();
        static Status GetStatus();
        static void WaitForState(State state);
        static bool Initialize();
        static bool Shutdown();

#define DEFINE_STATE_CHECK(Name) \
        static inline bool Is##Name(){ return GetState() == State::k##Name; }
        FOR_EACH_HEALTHCHECK_SERVER_STATE(DEFINE_STATE_CHECK)
#undef DEFINE_STATE_CHECK

#define DEFINE_STATUS_CHECK(Name) \
        static inline bool Is##Name(){ return GetStatus() == Status::k##Name; }
        FOR_EACH_HEALTHCHECK_SERVER_STATUS(DEFINE_STATUS_CHECK)
#undef DEFINE_STATUS_CHECK
    };
}

#endif //TOKEN_HEALTHCHECK_H