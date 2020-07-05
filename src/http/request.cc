#ifdef TOKEN_HEALTHCHECK_SUPPORT
#include <glog/logging.h>
#include "http/request.h"

namespace Token{
    int HttpRequest::OnParseUrl(http_parser* parser, const char* data, size_t len){
        HttpRequest* request = (HttpRequest*)parser->data;
        request->SetRoute(std::string(data, len));
        return 0;
    }

    void HttpRequest::SendResponse(Token::HttpResponse* response){
        std::stringstream stream;
        stream << (*response);
        std::string value = stream.str();

        uv_buf_t buff = uv_buf_init((char*)value.data(), value.size());
        uv_write_t* req = (uv_write_t*)malloc(sizeof(uv_write_t));
        uv_write(req, (uv_stream_t*)&handle_, &buff, 1, OnResponseSent);
    }

    void HttpRequest::OnResponseSent(uv_write_t* req, int status){
        if(status != 0)LOG(ERROR) << "failed to send response: " << uv_strerror(status);
        uv_close((uv_handle_t*)req->handle, OnClose);
        free(req);
    }

    void HttpRequest::OnClose(uv_handle_t* handle){
        HttpRequest* request = (HttpRequest*)handle->data;
        free(request);
    }
}

#endif//TOKEN_HEALTHCHECK_SUPPORT