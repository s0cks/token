#include <mutex>
#include <condition_variable>
#include "common.h"
#include "http/session.h"
#include "http/healthcheck.h"
#include "configuration.h"
#include "block_chain.h"

#include "server.h"

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
        router->Get("/status", &HandleStatusEndpoint);
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

    static HttpSession* sessions_[HealthCheckService::kMaxNumberOfSessions];

    bool HealthCheckService::RegisterSession(const Handle<HttpSession>& session){
        for(int64_t idx = 0; idx < HealthCheckService::kMaxNumberOfSessions; idx++){
            if(!sessions_[idx]){
                sessions_[idx] = session;
                return true;
            }
        }
        return false;
    }

    bool HealthCheckService::UnregisterSession(const Handle<HttpSession>& session){
        for(int64_t idx = 0; idx < HealthCheckService::kMaxNumberOfSessions; idx++){
            //TODO: fix HealthCheckService::UnregisterSession(const Handle<HttpSession>&);
            if(sessions_[idx] && sessions_[idx]->GetStartAddress() == session->GetStartAddress()){
                sessions_[idx] = nullptr;
                return true;
            }
        }
        return false;
    }

    bool HealthCheckService::Accept(WeakObjectPointerVisitor* vis){
        for(int64_t idx = 0; idx < HealthCheckService::kMaxNumberOfSessions; idx++){
            if(sessions_[idx] && !vis->Visit(&sessions_[idx])){
                LOG(WARNING) << "couldn't visit session #" << idx;
                return false;
            }
        }
        return true;
    }

    void HealthCheckService::OnNewConnection(uv_stream_t* stream, int status){
        Handle<HttpSession> session = HttpSession::NewInstance(stream->loop);
        RegisterSession(session);

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

    static inline void
    SendOk(HttpSession* session){
        LOG(INFO) << "sending Ok()";

        HttpResponse response(session, STATUS_CODE_OK, "Ok");
        session->Send(&response);
    }

    static inline void
    SendInternalServerError(HttpSession* session, const std::string& msg="Internal Server Error"){
        LOG(INFO) << "sending InternalServerError(" << msg << ")";

        std::stringstream ss;
        ss << "Internal Server Error: " << msg;
        HttpResponse response(session, STATUS_CODE_INTERNAL_SERVER_ERROR, ss);
        session->Send(&response);
    }

    static inline void
    SendNotFound(HttpSession* session, const std::string& path){
        LOG(WARNING) << "sending NotFound(" << path << ")";

        std::stringstream ss;
        ss << "Not Found: " << path;
        HttpResponse response(session, STATUS_CODE_NOTFOUND, ss);
        session->Send(&response);
    }

    static inline std::string
    Json2String(Json::Value& value){
        Json::StreamWriterBuilder builder;
        builder["commentStyle"] = "None";
        builder["indentation"] = "";
        std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
        std::ostringstream os;
        writer->write(value, &os);
        return os.str();
    }

    static inline void
    SendJson(HttpSession* session, Json::Value& value, int status=STATUS_CODE_OK){
        HttpResponse response(session, status, CONTENT_APPLICATION_JSON, Json2String(value));
        session->Send(&response);
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
        SendNotFound(session, request->GetPath());
    }

    void HealthCheckService::HandleReadyEndpoint(HttpSession* session, HttpRequest* request){
        SendOk(session);
    }

    void HealthCheckService::HandleLiveEndpoint(HttpSession* session, HttpRequest* request){
        SendOk(session);
    }

    static inline bool
    GetRuntimeStatus(Json::Value& value){
        value["Block Chain"] = BlockChain::GetStatusMessage();
        value["Server"] = Server::GetStatusMessage();
        value["Unclaimed Transaction Pool"] = UnclaimedTransactionPool::GetStatusMessage();
        return true;
    }

    static inline bool
    GetBlockChainHead(Json::Value& value){
        Handle<Block> head = BlockChain::GetHead();
        return head->ToJson(value);
    }

    void HealthCheckService::HandleStatusEndpoint(HttpSession* session, HttpRequest* request){
        Json::Value head;
        if(!GetBlockChainHead(head)){
            SendInternalServerError(session);
            return;
        }

        Json::Value status;
        if(!GetRuntimeStatus(status)){
            SendInternalServerError(session);
            return;
        }

        Json::Value doc;
        doc["head"] = BlockChain::GetReference(BLOCKCHAIN_REFERENCE_HEAD).HexString();
        doc["genesis"] = BlockChain::GetReference(BLOCKCHAIN_REFERENCE_GENESIS).HexString();
        doc["status"] = status;
        SendJson(session, doc);
    }
}