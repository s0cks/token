#ifndef TOKEN_SERVICE_H
#define TOKEN_SERVICE_H

#include <uv.h>
#include "http/session.h"
#include "http/request.h"
#include "http/response.h"

namespace Token{
  class HttpSession;
  class HttpService{
   protected:
    HttpService() = delete;

    static void AllocBuffer(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buff);
    static void OnClose(uv_handle_t* handle);
    static void OnWalk(uv_handle_t* handle, void* data);

    static void SendNotSupported(HttpSession* session, const HttpRequestPtr& request);
    static void SendNotFound(HttpSession* session, const HttpRequestPtr& request);

    static bool Bind(uv_tcp_t* server, const int32_t& port);
    static bool Accept(uv_stream_t* stream, HttpSession* session);
    static bool ShutdownService(uv_loop_t* loop);
   public:
    virtual ~HttpService() = delete;
  };
}

#endif //TOKEN_SERVICE_H