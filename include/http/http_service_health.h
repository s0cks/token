#ifndef TOKEN_HTTP_SERVICE_HEALTH_H
#define TOKEN_HTTP_SERVICE_HEALTH_H

#include "http/http_service.h"
#include "http/http_controller_health.h"

namespace token{
  class HttpHealthService : public HttpService{
   protected:
    std::shared_ptr<HealthController> health_;
   public:
    explicit HttpHealthService(uv_loop_t* loop=uv_loop_new());
    ~HttpHealthService() override = default;

    std::shared_ptr<HealthController> GetHealthController() const{
      return health_;
    }

    ServerPort GetPort() const override{
      return GetServerPort();
    }

    static inline const char*
    GetThreadName(){
      return "http/health";
    }

    static inline ServerPort
    GetServerPort(){
      return FLAGS_healthcheck_port;
    }

    static HttpHealthService* GetInstance();
  };

  class HttpHealthServiceThread{
   private:
    static void HandleThread(uword param);
   public:
    HttpHealthServiceThread() = delete;
    ~HttpHealthServiceThread() = delete;

    static bool Join();
    static bool Start();
  };
}

#endif//TOKEN_HTTP_SERVICE_HEALTH_H