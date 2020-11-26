#ifndef TOKEN_HEALTHCHECK_H
#define TOKEN_HEALTHCHECK_H

#include <uv.h>
#include <libconfig.h++>

#include "vthread.h"

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

#define FOR_EACH_HEALTHCHECK_ENDPOINT(V) \
    V(Live, "/live")                     \
    V(Ready, "/ready")                   \
    V(Health, "/health")

    enum class HealthCheckEndpoint{
#define DEFINE_HEALTHCHECK_ENDPOINT(Name, Path) k##Name##Endpoint,
        FOR_EACH_HEALTHCHECK_ENDPOINT(DEFINE_HEALTHCHECK_ENDPOINT)
#undef DEFINE_HEALTHCHECK_ENDPOINT
        kUnknownEndpoint,
    };

    static std::ostream& operator<<(std::ostream& stream, const HealthCheckEndpoint& endpoint){
        switch(endpoint){
#define DEFINE_TOSTRING(Name, Path) \
            case HealthCheckEndpoint::k##Name##Endpoint: \
                stream << Path; \
                return stream;
            FOR_EACH_HEALTHCHECK_ENDPOINT(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
            case HealthCheckEndpoint::kUnknownEndpoint:
            default:
                stream << "unknown";
                return stream;
        }
    }

    static inline HealthCheckEndpoint
    GetHealthCheckEndpoint(const std::string& path){
        LOG(INFO) << "path: " << path;
#define DEFINE_ENDPOINT_CHECK(Name, Path) \
        if(!strncmp(path.data(), Path, path.size())) \
            return HealthCheckEndpoint::k##Name##Endpoint;
        FOR_EACH_HEALTHCHECK_ENDPOINT(DEFINE_ENDPOINT_CHECK)
#undef DEFINE_ENDPOINT_CHECK
        return HealthCheckEndpoint::kUnknownEndpoint;
    }

    class HealthCheckService : public Thread{
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

        static void LoadConfiguration(libconfig::Setting& config);
        static void SaveConfiguration(libconfig::Setting& config);
        static void SetServicePort(int port);
        static void SetState(State state);
        static void SetStatus(Status status);
        static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
        static void OnNewConnection(uv_stream_t* stream, int status);
        static void OnShutdown(uv_shutdown_t* req, int status);
        static void OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff);
        static void HandleServiceThread(uword parameter);
    public:
        ~HealthCheckService() = delete;

        static int GetServicePort();
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