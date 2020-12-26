#include <mutex>
#include <condition_variable>
#include "configuration.h"
#include "http/controller.h"
#include "http/healthcheck.h"

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
        StatusController::Initialize(&router_);
        return Thread::Start("HealthCheckService", &HandleServiceThread, 0);
    }

    bool HealthCheckService::Stop(){
        if(!IsRunning())
            return true; // should we return false?
        uv_async_send(&shutdown_);

        int ret;
        if((ret = pthread_join(thread_, NULL)) != 0){
            LOG(WARNING) << "couldn't join the health check service thread: " << strerror(ret);
            return false;
        }
        return true;
    }

    void HealthCheckService::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        HttpSession* session = (HttpSession*)handle->data;
        session->InitReadBuffer(buff);
    }

    void HealthCheckService::HandleServiceThread(uword parameter){
        SetState(HealthCheckService::kStarting);
        uv_loop_t* loop = uv_loop_new();
        uv_tcp_t server;
        uv_tcp_init(loop, &server);

        uv_async_init(loop, &shutdown_, &OnShutdown);

        int32_t port = BlockChainConfiguration::GetHealthCheckPort();
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

    void HealthCheckService::OnClose(uv_handle_t* handle){

    }

    void HealthCheckService::OnWalk(uv_handle_t* handle, void* data){
        uv_close(handle, &OnClose);
    }

    void HealthCheckService::OnShutdown(uv_async_t* handle){
        SetState(HealthCheckService::kStopping);
        uv_stop(handle->loop);

        int err;
        if((err = uv_loop_close(handle->loop)) == UV_EBUSY)
            uv_walk(handle->loop, &OnWalk, NULL);

        uv_run(handle->loop, UV_RUN_DEFAULT);
        if((err = uv_loop_close(handle->loop)))
            LOG(ERROR) << "failed to close the server loop: " << uv_strerror(err);
    }

    void HealthCheckService::OnNewConnection(uv_stream_t* stream, int status){
        HttpSession* session = new HttpSession(stream->loop);

        LOG(INFO) << "client is connecting....";
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

            HttpRouterMatch match = router_.Find(&request);
            if(match.IsNotFound()){
                SendNotFound(session, request.GetPath());
            } else if(match.IsMethodNotSupported()){
                SendNotSupported(session, request.GetPath());
            } else{
                assert(match.IsOk());

                // weirdness :(
                request.SetParameters(match.GetParameters());
                HttpRouteHandler& handler = match.GetHandler();
                handler(session, &request);
            }
        } else{
            if(nread == UV_EOF){
                // fallthrough
            } else{
                LOG(WARNING) << "server read failure: " << uv_strerror(nread);
                session->Close();
                return;
            }
        }
    }

    void HealthCheckService::HandleUnknownEndpoint(HttpSession* session, HttpRequest* request){
        SendNotFound(session, request->GetPath());
    }
}