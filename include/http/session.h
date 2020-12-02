#ifndef TOKEN_HTTP_SESSION_H
#define TOKEN_HTTP_SESSION_H

#include <uv.h>
#include <glog/logging.h>
#include "buffer.h"
#include "http/request.h"
#include "uuid.h"

namespace Token{
    class HttpSession{
        friend class HealthCheckService;
    public:
        static const size_t kBufferSize = 4096;
    private:
        UUID session_id_;
        uv_tcp_t handle_;
        Buffer wbuff_;
        Buffer rbuff_;

        void InitReadBuffer(uv_buf_t* buff){
            Buffer* rbuff = GetReadBuffer();
            buff->len = rbuff->GetBufferSize();
            buff->base = rbuff->data();
        }

        void InitWriteBuffer(uv_buf_t* buff){
            Buffer* wbuff = GetWriteBuffer();
            buff->len = wbuff->GetBufferSize();
            buff->base = wbuff->data();
        }

        static void OnResponseSent(uv_write_t* req, int status);
        static void OnClose(uv_handle_t* handle);
    public:
        HttpSession(uv_loop_t* loop):
            session_id_(),
            handle_(),
            wbuff_(kBufferSize),
            rbuff_(kBufferSize){
            handle_.data = this;
            int err;
            if((err = uv_tcp_init(loop, &handle_)) != 0){
                LOG(WARNING) << "couldn't initialize the HttpSession::handle: " << uv_strerror(err);
                return;
            }
        }
        ~HttpSession(){
            LOG(ERROR) << "destroying http session";
            Close();
        }

        UUID GetSessionID() const{
            return session_id_;
        }

        uv_stream_t* GetStream() const{
            return (uv_stream_t*)&handle_;
        }

        Buffer* GetReadBuffer(){
            return &rbuff_;
        }

        Buffer* GetWriteBuffer(){
            return &wbuff_;
        }

        void Send(HttpResponse* response);
        void Close();

        std::string ToString() const{
            return "HttpSession()";
        }
    };
}

#endif //TOKEN_HTTP_SESSION_H