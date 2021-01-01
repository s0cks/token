#ifndef TOKEN_HTTP_SESSION_H
#define TOKEN_HTTP_SESSION_H

#include <uv.h>
#include <glog/logging.h>
#include "uuid.h"
#include "utils/buffer.h"
#include "http/request.h"


namespace Token{
    class HttpSession{
        friend class HttpService;
    private:
        UUID session_id_;
        uv_tcp_t handle_;
        BufferPtr rbuff_;
        BufferPtr wbuff_;

        void InitReadBuffer(uv_buf_t* buff, int64_t suggested_size){
            rbuff_ = Buffer::NewInstance(suggested_size);
            buff->len = rbuff_->GetBufferSize();
            buff->base = rbuff_->data();
        }

        void InitWriteBuffer(uv_buf_t* buff, int64_t size){
            wbuff_ = Buffer::NewInstance(size);
            buff->base = wbuff_->data();
            buff->len = wbuff_->GetBufferSize();
        }

        static void OnResponseSent(uv_write_t* req, int status);
        static void OnClose(uv_handle_t* handle);
    public:
        HttpSession(uv_loop_t* loop):
            session_id_(),
            handle_(),
            rbuff_(BufferPtr(nullptr)),
            wbuff_(BufferPtr(nullptr)){
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

        UUID GetID() const{
            return session_id_;
        }

        uv_stream_t* GetStream() const{
            return (uv_stream_t*)&handle_;
        }

        std::string ToString() const{
            std::stringstream ss;
            ss << "HttpSession(" << GetID() << ")";
            return ss.str();
        }

        void Send(HttpResponse* response);
        void Close();
    };
}

#endif //TOKEN_HTTP_SESSION_H