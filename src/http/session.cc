#include "http/session.h"

namespace Token{
    void HttpSession::Send(HttpResponse* response){
        std::stringstream ss;
        ss << "HTTP/1.1 200 OK\r\n";
        ss << "Content-Type: text/plain\r\n";
        ss << "Content-Length: " << response->GetContentLength() << "\r\n";
        ss << "\r\n";
        ss << response->GetBody();
        std::string resp = ss.str();

        Handle<Buffer> buffer = GetWriteBuffer();
        buffer->PutString(ss.str());

        uv_buf_t buff;
        buffer->Initialize(&buff);

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
        uv_close((uv_handle_t*)&handle_, OnClose);
    }

    void HttpSession::OnClose(uv_handle_t* handle){}
}