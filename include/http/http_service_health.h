#ifndef TOKEN_HTTP_SERVICE_HEALTH_H
#define TOKEN_HTTP_SERVICE_HEALTH_H

#ifdef TOKEN_ENABLE_HEALTH_SERVICE

#include "http/http_service.h"

namespace Token{
  class HttpHealthService : HttpService{
   public:
    HttpHealthService(uv_loop_t* loop=uv_loop_new());
    ~HttpHealthService() = default;

    ServerPort GetPort() const{
      return FLAGS_healthcheck_port;
    }

    static bool Start();
    static bool Shutdown();
    static bool WaitForShutdown();

#define DECLARE_STATE_CHECK(Name) \
    static bool IsService##Name();
    FOR_EACH_SERVER_STATE(DECLARE_STATE_CHECK)
#undef DECLARE_STATE_CHECK
  };
}

#endif//TOKEN_ENABLE_HEALTH_SERVICE
#endif//TOKEN_HTTP_SERVICE_HEALTH_H