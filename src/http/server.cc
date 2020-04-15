#include "http/server.h"

namespace Token{
    static uv_tcp_t server;
    static pthread_t thread;

    //TODO:
    // - create HttpRequest & HttpResponse classes
    // - create skeleton for http endpoint handling
    // - offload work from nodejs to http service
    // - re-arrange port layout
    // - make configurable via flag
    void* BlockChainHttpServer::ServerThread(void* data){
        uv_loop_t* loop = uv_loop_new();
        sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", 8080, &addr);
        uv_tcp_init(loop, &server);
        uv_tcp_bind(&server, (struct sockaddr*)&addr, 0);
        int err;
        if((err = uv_listen((uv_stream_t*)&server, 100, &OnNewConnection)) != 0){
            LOG(ERROR) << "couldn't start server: " << uv_strerror(err);
            pthread_exit(nullptr);
        }
        uv_run(loop, UV_RUN_DEFAULT);
        pthread_exit(nullptr);
    }

    bool BlockChainHttpServer::StartServer(){
        return pthread_create(&thread, NULL, &ServerThread, NULL) == 0;
    }

    void BlockChainHttpServer::AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff){
        buff->base = (char*)malloc(suggested_size);
        buff->len = suggested_size;
    }

    void BlockChainHttpServer::OnNewConnection(uv_stream_t *stream, int status){
        HttpRequest* request = new HttpRequest();
        int r = uv_tcp_init(stream->loop, &request->handle_);
        if((r = uv_accept(stream, (uv_stream_t*)&request->handle_))){
            LOG(ERROR) << "client accept failure: " << uv_strerror(r);
            return;
        }
        r = uv_read_start((uv_stream_t*)&request->handle_, &AllocBuffer, &OnMessageReceived);
    }

    void BlockChainHttpServer::HandleHealthRequest(uv_work_t* req){
        HttpRequest* request = (HttpRequest*)req->data;
        HttpResponse response(200, "Hello World");
        request->SendResponse(&response);
    }

    void BlockChainHttpServer::AfterHandleRequest(uv_work_t* req, int status){
        free(req);
    }

    void BlockChainHttpServer::OnMessageReceived(uv_stream_t *stream, ssize_t nread, const uv_buf_t* buff){
        HttpRequest* request = (HttpRequest*)stream->data;
        int r = 0;
        if(nread >= 0){
            size_t parsed = request->Parse(buff->base, buff->len);
            if(parsed < nread){
                LOG(ERROR) << "parser error: " << request->GetError();
                return;
            }
            uv_work_t* work = (uv_work_t*)malloc(sizeof(uv_work_t));
            work->data = request;
            if(BeginsWith(request->GetRoute(), "/status")){
                uv_queue_work(stream->loop, work, HandleHealthRequest, AfterHandleRequest);
            } else{
                LOG(ERROR) << "unknown route: " << request->GetRoute();
                uv_close((uv_handle_t*)stream, nullptr);
                return;
            }
        } else{
            if(nread == UV_EOF){
                //fallthrough
            } else{
                LOG(ERROR) << "read: " << uv_strerror(r);
                uv_close((uv_handle_t*)stream, nullptr);
                return;
            }
        }
        free(buff->base);
    }
}