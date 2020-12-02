#include "http/session.h"
#include "http/healthcheck.h"

namespace Token{
    void HttpSession::Send(HttpResponse* response){
        std::stringstream ss;
        ss << "HTTP/1.1 200 OK\r\n";
        ss << "Content-Type: text/plain\r\n";
        ss << "Content-Length: " << response->GetContentLength() << "\r\n";
        ss << "\r\n";
        ss << response->GetBody();
        std::string resp = ss.str();

        Buffer* wbuff = GetWriteBuffer();
        wbuff->PutString(ss.str());

        uv_buf_t buff;
        buff.len = wbuff->GetBufferSize();
        buff.base = wbuff->data();

        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        req->data = this;
        uv_write(req, (uv_stream_t*)&handle_, &buff, 1, &OnResponseSent);
    }

    void HttpSession::OnResponseSent(uv_write_t* req, int status){
        HttpSession* session = (HttpSession*)req->data;
        if(status != 0)
            LOG(WARNING) << "failed to send the response: " << uv_strerror(status);
        free(req);
        session->Close();
    }

    void HttpSession::Close(){
        if(uv_is_closing((uv_handle_t*)&handle_))
            return;
        uv_close((uv_handle_t*)&handle_, OnClose);
    }

    void HttpSession::OnClose(uv_handle_t* handle){
        HttpSession* session = (HttpSession*)handle->data;
        if(!HealthCheckService::UnregisterSession(session))
            LOG(WARNING) << "couldn't unregister http session from health check service";
    }
}