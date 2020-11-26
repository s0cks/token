#ifndef TOKEN_SESSION_H
#define TOKEN_SESSION_H

#include <uv.h>
#include <glog/logging.h>
#include "http/request.h"

namespace Token{
    class HttpSession{
    public:
        static const size_t kBufferSize = 4096;
    private:
        uv_tcp_t handle_;

        static void OnResponseSent(uv_write_t* req, int status);
        static void OnClose(uv_handle_t* handle);
    public:
        HttpSession(uv_loop_t* loop):
            handle_(){
            handle_.data = this;
            int err;
            if((err = uv_tcp_init(loop, &handle_)) != 0){
                LOG(WARNING) << "couldn't initialize the HttpSession::handle: " << uv_strerror(err);
                return;
            }
        }
        ~HttpSession() = default;

        uv_stream_t* GetStream() const{
            return (uv_stream_t*)&handle_;
        }

        void Send(HttpResponse* response);
        void Close();
    };
}

#endif //TOKEN_SESSION_H