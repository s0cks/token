#ifndef TOKEN_HEALTHCHECK_H
#define TOKEN_HEALTHCHECK_H

#include <uv.h>
#include <libconfig.h++>

#include "vthread.h"
#include "http/router.h"
#include "http/session.h"
#include "http/request.h"
#include "http/service.h"
#include "http/controller.h"

namespace Token{
    class StatusController : HttpController{
    private:
        StatusController() = delete;

        static void HandleOverallStatus(HttpSession* session, HttpRequest* request);
        static void HandleTestStatus(HttpSession* session, HttpRequest* request);
    public:
        ~StatusController() = delete;

        static inline bool
        Initialize(HttpRouter* router){
            router->Get("/status", &HandleOverallStatus);
            router->Get("/status/:name", &HandleTestStatus);
            return true;
        }
    };

    class HealthController : HttpController{
    private:
        HealthController() = delete;

        static void HandleReadyEndpoint(HttpSession* session, HttpRequest* request){
            SendOk(session);
        }

        static void HandleLiveEndpoint(HttpSession* session, HttpRequest* request){
            SendOk(session);
        }
    public:
        ~HealthController() = delete;

        static inline bool
        Initialize(HttpRouter* router){
            router->Get("/ready", &HandleReadyEndpoint);
            router->Get("/live", &HandleLiveEndpoint);
            return true;
        }
    };

#define FOR_EACH_HEALTHCHECK_SERVER_STATE(V) \
    V(Starting)                 \
    V(Running)                  \
    V(Stopping)                 \
    V(Stopped)

#define FOR_EACH_HEALTHCHECK_SERVER_STATUS(V) \
    V(Ok)                                     \
    V(Warning)                                \
    V(Error)

    class HealthCheckService : Thread, HttpService{
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
    private:
        HealthCheckService() = delete;

        static void SetState(State state);
        static void SetStatus(Status status);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        static void OnShutdown(uv_async_t* handle);
        static void HandleServiceThread(uword parameter);
    public:
        ~HealthCheckService() = delete;

        static void WaitForState(State state);
        static State GetState();
        static Status GetStatus();
        static bool Start();
        static bool Stop();

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