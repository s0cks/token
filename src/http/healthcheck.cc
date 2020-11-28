#include <mutex>
#include <condition_variable>
#include "common.h"
#include "http/session.h"
#include "http/healthcheck.h"
#include "configuration.h"
#include "block_chain.h"

namespace Token{
    static std::mutex mutex_;
    static std::condition_variable cond_;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static HealthCheckService::State state_ = HealthCheckService::kStopped;
    static HealthCheckService::Status status_ = HealthCheckService::kOk;
    static HttpRouter* router_;

    HttpRouter* HealthCheckService::GetRouter(){
        LOCK_GUARD;
        return router_;
    }

    void HealthCheckService::SetRouter(HttpRouter* router){
        LOCK_GUARD;
        router_ = router;
    }

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

    bool HealthCheckService::Initialize(){
        if(!IsStopped()){
            LOG(WARNING) << "the health check service is already running.";
            return false;
        }
        LOG(INFO) << "initializing the health check service....";
        HttpRouter* router = new HttpRouter(&HandleUnknownEndpoint);
        router->Get("/ready", &HandleReadyEndpoint);
        router->Get("/live", &HandleLiveEndpoint);
        router->Get("/health", &HandleHealthEndpoint);
        SetRouter(router);
        return Thread::Start("HealthCheckService", &HandleServiceThread, 0);
    }

    bool HealthCheckService::Shutdown(){
        //TODO: implement HealthCheckService::Shutdown()
        return false;
    }

    void HealthCheckService::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        Handle<HttpSession> session = (HttpSession*)handle->data;
        session->InitReadBuffer(buff);
    }

    void HealthCheckService::HandleServiceThread(uword parameter){
        SetState(HealthCheckService::kStarting);
        uv_loop_t* loop = uv_loop_new();
        uv_tcp_t server;
        uv_tcp_init(loop, &server);

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

    void HealthCheckService::OnShutdown(uv_shutdown_t* req, int status){

    }

    void HealthCheckService::OnNewConnection(uv_stream_t* stream, int status){
        LOG(INFO) << "client is connecting....";
        Handle<HttpSession> session = HttpSession::NewInstance(stream->loop);

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
            HttpRouter::HttpRoute route = GetRouter()->GetRouteOrDefault(&request);
            route(session, &request);
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
        std::stringstream ss;
        ss << "Unknown Endpoint: " << request->GetPath();
        HttpResponse response(session, 404, ss);
        session->Send(&response);
    }

    void HealthCheckService::HandleReadyEndpoint(HttpSession* session, HttpRequest* request){
        std::stringstream ss;
        ss << "Ok";

        HttpResponse response(session, 200, ss);
        session->Send(&response);
    }

    void HealthCheckService::HandleLiveEndpoint(HttpSession* session, HttpRequest* request){
        LOG(INFO) << "handling /live endpoint....";

        std::stringstream ss;
        ss << "Ok";

        HttpResponse response(session, 200, ss);
        session->Send(&response);
    }

    static inline std::string
    Block2Json(const Handle<Block>& blk){
        std::stringstream ss;
        ss << "{";
        ss << "\"timestamp\": " << blk->GetTimestamp() << ",";
        ss << "\"height\": " << blk->GetHeight() << ",";
        ss << "\"previous_hash\": \"" << blk->GetPreviousHash() << "\",";
        ss << "\"merkle_root\": \"" << blk->GetMerkleRoot() << "\",";
        ss << "\"hash\": \"" << blk->GetHash() << "\"";
        ss << "}";
        return ss.str();
    }

    void HealthCheckService::HandleHealthEndpoint(HttpSession* session, HttpRequest* request){
        std::stringstream ss;
        ss << "{";
        ss << "\"head\": " << Block2Json(BlockChain::GetHead());
        ss << "}";
        HttpResponse response(session, 200, CONTENT_APPLICATION_JSON, ss);
        session->Send(&response);
    }
}