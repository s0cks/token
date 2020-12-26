#include <mutex>
#include <condition_variable>

#include "http/rest.h"
#include "blockchain.h"

namespace Token{
    static pthread_t thread_;
    static std::mutex mutex_;
    static std::condition_variable cond_;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define UNLOCK lock.unlock()
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static RestService::State state_ = RestService::kStopped;
    static RestService::Status status_ = RestService::kOk;
    static HttpRouter router_;
    static uv_async_t shutdown_;

    RestService::State RestService::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void RestService::SetState(State state){
        LOCK;
        state_ = state;
        UNLOCK;
        SIGNAL_ALL;
    }

    void RestService::WaitForState(State state){
        LOCK;
        while(state_ != state) WAIT;
        UNLOCK;
    }

    RestService::Status RestService::GetStatus(){
        LOCK_GUARD;
        return status_;
    }

    void RestService::SetStatus(Status status){
        LOCK;
        status_ = status;
        UNLOCK;
    }

    bool RestService::Start(){
        if(!IsStopped()){
            LOG(WARNING) << "the rest service is already running.";
            return false;
        }

        LOG(INFO) << "starting the rest service....";
        RestController::Initialize(&router_);
        return Thread::Start(&thread_, "RestService", &HandleServiceThread, 0);
    }

    void RestService::OnShutdown(uv_async_t* handle){
        SetState(RestService::kStopped);
        uv_stop(handle->loop);
        if(!HttpService::ShutdownService(handle->loop))
            LOG(WARNING) << "couldn't shutdown the HealthCheck service";
    }

    bool RestService::Stop(){
        if(!IsRunning())
            return true; // should we return false?
        uv_async_send(&shutdown_);
        return Thread::Stop(thread_);
    }

    void RestService::HandleServiceThread(uword parameter){
        SetState(RestService::kStarting);
        uv_loop_t* loop = uv_loop_new();
        uv_async_init(loop, &shutdown_, &OnShutdown);

        uv_tcp_t server;
        uv_tcp_init(loop, &server);

        int32_t port = FLAGS_port + 2; //TODO: fixme
        if(!Bind(&server, port)){
            LOG(WARNING) << "couldn't bind the rest service on port: " << port;
            goto exit;
        }

        int result;
        if((result = uv_listen((uv_stream_t*)&server, 100, &OnNewConnection)) != 0){
            LOG(WARNING) << "the rest service couldn't listen on port " << port << ": " << uv_strerror(result);
            goto exit;
        }

        LOG(INFO) << "rest service listening @" << port;
        SetState(State::kRunning);
        uv_run(loop, UV_RUN_DEFAULT);
    exit:
        uv_loop_close(loop);
        pthread_exit(nullptr);
    }

    void RestService::OnNewConnection(uv_stream_t* stream, int status){
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

    void RestService::OnMessageReceived(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buff){
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
            std::stringstream ss;
            ss << "Not Found: " << request.GetPath();
            std::string body = ss.str();
            HttpResponse response(session, STATUS_CODE_NOTFOUND, body);
            response.SetHeader("Content-Type", CONTENT_TYPE_TEXT_PLAIN);
            response.SetHeader("Content-Length", body.size());
            session->Send(&response);
        } else if(match.IsMethodNotSupported()){
            std::stringstream ss;
            ss << "Not Supported.";
            std::string body = ss.str();
            HttpResponse response(session, STATUS_CODE_NOTSUPPORTED, body);
            response.SetHeader("Content-Type", CONTENT_TYPE_TEXT_PLAIN);
            response.SetHeader("Content-Length", body.size());
            session->Send(&response);
        } else{
            assert(match.IsOk());

            // weirdness :(
            request.SetParameters(match.GetParameters());
            HttpRouteHandler& handler = match.GetHandler();
            handler(session, &request);
        }
    }

    void RestController::HandleHead(HttpSession* session, HttpRequest* request){
        BlockPtr blk = BlockChain::GetHead();

        Json::Value doc;
        if(!blk->ToJson(doc)){
            SendInternalServerError(session, "Cannot convert <HEAD> to json");
            return;
        }

        SendJson(session, doc);
    }
}