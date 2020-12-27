#include <mutex>
#include <condition_variable>
#include "configuration.h"
#include "http/controller.h"
#include "http/healthcheck_service.h"

namespace Token{
    static pthread_t thread_;
    static std::mutex mutex_;
    static std::condition_variable cond_;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static HealthCheckService::State state_ = HealthCheckService::kStopped;
    static HealthCheckService::Status status_ = HealthCheckService::kOk;
    static HttpRouter router_;
    static uv_async_t shutdown_;

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

    void HealthCheckService::WaitForState(State state){
        LOCK;
        while(state_ != state) WAIT;
    }

    bool HealthCheckService::Start(){
        if(!IsStopped()){
            LOG(WARNING) << "the health check service is already running.";
            return false;
        }
        LOG(INFO) << "initializing the health check service....";
        HealthController::Initialize(&router_);
        return Thread::Start(&thread_, "HealthCheckService", &HandleServiceThread, 0);
    }

    bool HealthCheckService::Stop(){
        if(!IsRunning())
            return true; // should we return false?
        uv_async_send(&shutdown_);
        return Thread::Stop(thread_);
    }

    void HealthCheckService::HandleServiceThread(uword parameter){
        SetState(HealthCheckService::kStarting);
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &shutdown_, &OnShutdown);

        uv_tcp_t server;
        uv_tcp_init(loop, &server);

        int32_t port = BlockChainConfiguration::GetHealthCheckPort();
        if(!Bind(&server, port)){
            LOG(WARNING) << "couldn't bind health check service on port: " << port;
            goto exit;
        }

        int result;
        if((result = uv_listen((uv_stream_t*)&server, 100, &OnNewConnection)) != 0){
            LOG(WARNING) << "health check service couldn't listen on port " << port << ": " << uv_strerror(result);
            goto exit;
        }

        LOG(INFO) << "health check service listening @" << port;
        SetState(State::kRunning);
        uv_run(loop, UV_RUN_DEFAULT);
    exit:
        uv_loop_close(loop);
        pthread_exit(nullptr);
    }

    void HealthCheckService::OnShutdown(uv_async_t* handle){
        SetState(HealthCheckService::kStopping);
        uv_stop(handle->loop);
        if(!HttpService::ShutdownService(handle->loop))
            LOG(WARNING) << "couldn't shutdown the HealthCheck service";
    }

    void HealthCheckService::OnNewConnection(uv_stream_t* stream, int status){
        HttpSession* session = new HttpSession(stream->loop);
        if(!Accept(stream, session)){
            LOG(WARNING) << "couldn't accept new connection.";
            return;
        }

        int result;
        if((result = uv_read_start(session->GetStream(), &AllocBuffer, &OnMessageReceived)) != 0){
            LOG(WARNING) << "server read failure: " << uv_strerror(result);
            return;
        }
    }

    void HealthCheckService::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
        HttpSession* session = (HttpSession*)stream->data;
        if(nread == UV_EOF)
            return;

        if(nread < 0){
            LOG(WARNING) << "server read failure: " << uv_strerror(nread);
            session->Close();
            return;
        }

        HttpRequest request(session, buff->base, buff->len);
        HttpRouterMatch match = router_.Find(&request);
        if(match.IsNotFound()){
            SendNotFound(session, &request);
        } else if(match.IsMethodNotSupported()){
            SendNotSupported(session, &request);
        } else{
            assert(match.IsOk());

            // weirdness :(
            request.SetParameters(match.GetParameters());
            HttpRouteHandler& handler = match.GetHandler();
            handler(session, &request);
        }
    }

    void HealthController::HandleGetReadyStatus(HttpSession* session, HttpRequest* request){
        SendOk(session); //TODO: implement
    }

    void HealthController::HandleGetLiveStatus(HttpSession* session, HttpRequest* request){
        SendOk(session); //TODO: implement
    }
}