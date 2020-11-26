#include <mutex>
#include <condition_variable>
#include "common.h"
#include "http/session.h"
#include "http/healthcheck.h"

#include "configuration.h"

namespace Token{
#define CONFIG_KEY_HEALTHCHECK_SERVICE "HealthCheck"
#define CONFIG_KEY_SERVICE_PORT "Port"

    static std::mutex mutex_;
    static std::condition_variable cond_;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static HealthCheckService::State state_ = HealthCheckService::kStopped;
    static HealthCheckService::Status status_ = HealthCheckService::kOk;
    static unsigned int service_port_ = 0;

    HealthCheckService::State HealthCheckService::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void HealthCheckService::SetState(State state){
        LOCK_GUARD;
        state_ = state;
    }

    HealthCheckService::Status HealthCheckService::GetStatus(){
        LOCK_GUARD;
        return status_;
    }

    void HealthCheckService::SetStatus(Status status){
        LOCK_GUARD;
        status_ = status;
    }

    int HealthCheckService::GetServicePort(){
        LOCK_GUARD;
        return service_port_;
    }

    void HealthCheckService::SetServicePort(int port){
        LOCK_GUARD;
        service_port_ = port;
    }

    void HealthCheckService::WaitForState(State state){
        LOCK;
        while(state_ != state) WAIT;
    }

    bool HealthCheckService::Initialize(){
        if(!IsStopped()){
            LOG(WARNING) << "the health check service is already running.";
            return false;
        }

        LOG(INFO) << "initializing the health check service....";
        libconfig::Setting& config = BlockChainConfiguration::GetProperty(CONFIG_KEY_HEALTHCHECK_SERVICE, libconfig::Setting::TypeGroup);
        LoadConfiguration(config);
        return Thread::Start("HealthCheckService", &HandleServiceThread, 0);
    }

    bool HealthCheckService::Shutdown(){
        //TODO: implement HealthCheckService::Shutdown()
        return false;
    }

    void HealthCheckService::LoadConfiguration(libconfig::Setting& config){
        if(!config.exists(CONFIG_KEY_SERVICE_PORT)){
            int port = FLAGS_port + 1;
            SetServicePort(port);
            config.add(CONFIG_KEY_SERVICE_PORT, libconfig::Setting::TypeInt) = port;
            BlockChainConfiguration::SaveConfiguration();
        } else{
            int port;
            config.lookupValue(CONFIG_KEY_SERVICE_PORT, port);
            SetServicePort(port);
        }
    }

    void HealthCheckService::SaveConfiguration(libconfig::Setting& config){
        LOCK_GUARD;
        //TODO: implement HealthCheckService::SaveConfiguration(libconfig::Setting&)
    }

    void HealthCheckService::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        //TODO: buff->base = (char*)Allocator::Allocate(kBufferSize);
        buff->base = (char*)malloc(HttpSession::kBufferSize);
        buff->len = HttpSession::kBufferSize;
    }

    void HealthCheckService::HandleServiceThread(uword parameter){
        int port = GetServicePort();

        SetState(HealthCheckService::kStarting);
        uv_loop_t* loop = uv_loop_new();
        uv_tcp_t server;
        uv_tcp_init(loop, &server);

        sockaddr_in bind_address;
        uv_ip4_addr("0.0.0.0", port, &bind_address);
        uv_tcp_init(loop, &server);
        int err;
        if((err = uv_tcp_bind(&server, (struct sockaddr*)&bind_address, 0)) != 0){
            LOG(WARNING) << "couldn't bind the health check service on port " << port<< ": " << uv_strerror(err);
            goto exit;
        }

        if((err = uv_listen((uv_stream_t*)&server, 100, &OnNewConnection)) != 0){
            LOG(WARNING) << "health check service couldn't listen on port: ";
            goto exit;
        }

        LOG(INFO) << "health check service listening @" << port;
        SetState(State::kRunning);
        uv_run(loop, UV_RUN_DEFAULT);
    exit:
        uv_loop_close(loop);
        pthread_exit(nullptr);
    }

    void HealthCheckService::OnShutdown(uv_shutdown_t* req, int status){

    }

    void HealthCheckService::OnNewConnection(uv_stream_t* stream, int status){
        LOG(INFO) << "client is connecting....";
        HttpSession* session = new HttpSession(stream->loop);

        int err;
        if((err = uv_accept(stream, session->GetStream())) != 0){
            LOG(WARNING) << "server accept failure: " << uv_strerror(err);
            return;
        }

        LOG(INFO) << "client is connected.";
        if((err = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
            LOG(WARNING) << "server read failure: " << uv_strerror(err);
            return;
        }
    }

    void HealthCheckService::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        HttpSession* session = (HttpSession*)stream->data;
        if(nread >= 0){
            HttpRequest request(session, buff->base, buff->len);
            HealthCheckEndpoint endpoint = GetHealthCheckEndpoint(request.GetPath());
            switch(endpoint){
                case HealthCheckEndpoint::kReadyEndpoint:{
                    HttpResponse response(session, 200, "Hello World");
                    session->Send(&response);
                    //TODO: return ready status
                    return;
                }
                case HealthCheckEndpoint::kLiveEndpoint:
                    //TODO: return live status
                    return;
                case HealthCheckEndpoint::kHealthEndpoint:
                    //TODO: return health status
                    return;
                case HealthCheckEndpoint::kUnknownEndpoint:
                default:
                    //TODO: return 404
                    return;
            }
        } else{
            if(nread == UV_EOF){
                // fallthrough
            } else{
                LOG(WARNING) << "server read failure: " << uv_strerror(nread);
                uv_close((uv_handle_t*)stream, nullptr);
                return;
            }
        }
    }
}