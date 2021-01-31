#ifndef TOKEN_HTTP_SERVICE_REST_H
#define TOKEN_HTTP_SERVICE_REST_H

#ifdef TOKEN_ENABLE_REST_SERVICE

#include "http/http_service.h"

namespace Token{
  class HttpRestService : HttpService{
   public:
    HttpRestService(uv_loop_t* loop=uv_loop_new());
    ~HttpRestService() = default;

    ServerPort GetPort() const{
      return FLAGS_service_port;
    }

    static bool Start();
    static bool Shutdown();
    static bool WaitForShutdown();
    static HttpRouter* GetServiceRouter();

#define DECLARE_STATE_CHECK(Name) \
    static bool IsService##Name();
    FOR_EACH_SERVER_STATE(DECLARE_STATE_CHECK)
#undef DEFINE_STATE_CHECK
  };
}

#endif//TOKEN_ENABLE_REST_SERVICE
#endif//TOKEN_HTTP_SERVICE_REST_H