#ifndef TOKEN_HTTP_SESSION_H
#define TOKEN_HTTP_SESSION_H

#include <uv.h>
#include <glog/logging.h>
#include "buffer.h"
#include "http/request.h"

namespace Token{
    class HttpSession : public Object{
        friend class HealthCheckService;
    public:
        static const size_t kBufferSize = 4096;
    private:
        uv_tcp_t handle_;
        Buffer* read_buffer_;
        Buffer* write_buffer_;

        void InitReadBuffer(uv_buf_t* buff){
            Handle<Buffer> rbuff = GetReadBuffer();
            buff->len = rbuff->GetBufferSize();
            buff->base = rbuff->data();
        }

        void InitWriteBuffer(uv_buf_t* buff){
            Handle<Buffer> wbuff = GetWriteBuffer();
            buff->len = wbuff->GetBufferSize();
            buff->base = wbuff->data();
        }

        static void OnResponseSent(uv_write_t* req, int status);
        static void OnClose(uv_handle_t* handle);
    protected:
        HttpSession(uv_loop_t* loop):
            Object(),
            handle_(),
            read_buffer_(nullptr),
            write_buffer_(nullptr){
            SetType(Type::kHttpSessionType);

            handle_.data = this;
            int err;
            if((err = uv_tcp_init(loop, &handle_)) != 0){
                LOG(WARNING) << "couldn't initialize the HttpSession::handle: " << uv_strerror(err);
                return;
            }

            WriteBarrier(&read_buffer_, Buffer::NewInstance(kBufferSize));
            WriteBarrier(&write_buffer_, Buffer::NewInstance(kBufferSize));
        }

        bool Accept(WeakObjectPointerVisitor* vis){
            if(!vis->Visit(&read_buffer_)){
                LOG(ERROR) << "couldn't visit the read buffer.";
                return false;
            }

            if(!vis->Visit(&write_buffer_)){
                LOG(ERROR) << "couldn't visit the write buffer.";
                return false;
            }
            return true;
        }
    public:
        ~HttpSession(){
            LOG(ERROR) << "destroying http session";
            Close();
        }

        uv_stream_t* GetStream() const{
            return (uv_stream_t*)&handle_;
        }

        Handle<Buffer> GetReadBuffer() const{
            return read_buffer_;
        }

        Handle<Buffer> GetWriteBuffer() const{
            return write_buffer_;
        }

        void Send(HttpResponse* response);
        void Close();

        std::string ToString() const{
            return "HttpSession()";
        }

        static Handle<HttpSession> NewInstance(uv_loop_t* loop){
            return new HttpSession(loop);
        }
    };
}

#endif //TOKEN_HTTP_SESSION_H