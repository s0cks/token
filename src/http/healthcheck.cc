#include <mutex>
#include <condition_variable>

#include "server.h"
#include "block_chain.h"
#include "configuration.h"
#include "http/healthcheck.h"
#include "peer/peer_session_manager.h"

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
#define DEFINE_ENDPOINT_PATH(Method, Name, Path) router->Method(Path, &Handle##Name##Endpoint);
        FOR_EACH_HEALTHCHECK_SERVICE_ENDPOINT(DEFINE_ENDPOINT_PATH)
#undef DEFINE_ENDPOINT_PATH
        SetRouter(router);
        return Thread::Start("HealthCheckService", &HandleServiceThread, 0);
    }

    bool HealthCheckService::Shutdown(){
        //TODO: implement HealthCheckService::Shutdown()
        return false;
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

    void HealthCheckService::HandleBlockChainStatusEndpoint(HttpSession* session, HttpRequest* request){
        Json::Value response;
        response["Head"] = BlockChain::GetReference(BLOCKCHAIN_REFERENCE_HEAD).HexString();
        response["Genesis"] = BlockChain::GetReference(BLOCKCHAIN_REFERENCE_GENESIS).HexString();
        SendJson(session, response);
    }

    void HealthCheckService::HandlePeerStatusEndpoint(HttpSession* session, HttpRequest* request){
        Json::Value response;

        std::vector<std::string> status;
        if(!PeerSessionManager::GetStatus(status)){
            response["message"] = "cannot get the peer statuses";
            SendJson(session, response);
            return;
        }

        for(auto& it : status)
            response.append(Json::Value(it));
        SendJson(session, response);
        return;
    }

    void GetRuntimeStatus(Json::Value& value){
        value["Block Chain"] = BlockChain::GetStatusMessage();
        value["Server"] = Server::GetStatusMessage();
        value["Unclaimed Transaction Pool"] = UnclaimedTransactionPool::GetStatusMessage();
        value["Transaction Pool"] = TransactionPool::GetStatusMessage();
        value["Block Pool"] = BlockPool::GetStatusMessage();

        std::vector<std::string> status;
        if(!PeerSessionManager::GetStatus(status))
            return;

        Json::Value peers;
        for(auto& it : status)
            peers.append(Json::Value(it));
        value["Peers"] = peers;
    }

    void HealthCheckService::HandleInfoEndpoint(HttpSession* session, HttpRequest* request){
        Json::Value doc;
        doc["Timestamp"] = GetTimestampFormattedReadable(GetCurrentTimestamp());
        doc["Version"] = GetVersion();
        doc["Head"] = BlockChain::GetReference(BLOCKCHAIN_REFERENCE_HEAD).HexString();

        Json::Value runtime;
        GetRuntimeStatus(runtime);

        doc["Runtime"] = runtime;
        SendJson(session, doc);
    }
}