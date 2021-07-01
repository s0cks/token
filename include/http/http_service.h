#ifndef TOKEN_HTTP_SERVICE_H
#define TOKEN_HTTP_SERVICE_H

#include "server.h"
#include "http/http_router.h"
#include "http/http_message.h"
#include "http/http_session.h"

namespace token{
  namespace http{
    class ServiceBase : public ServerBase<http::Session>{
     protected:
      RouterPtr router_;

      ServiceBase(uv_loop_t* loop):
        ServerBase(loop),
        router_(Router::NewInstance()){}

      std::shared_ptr<http::Session> CreateSession() const override{
        return std::make_shared<Session>(GetLoop(), GetRouter());
      }
     public:
      ~ServiceBase() override = default;

      RouterPtr GetRouter() const{
        return router_;
      }
    };

    template<class Service>
    class ServiceThread{
     protected:
      ThreadId thread_;

      static void
      HandleThread(uword param){
        auto instance = Service::NewInstance();
        DLOG_THREAD_IF(ERROR, !instance->Run(Service::GetPort())) << "Failed to run the " << Service::GetName() << " service loop.";
        pthread_exit(nullptr);
      }
     public:
      ServiceThread() = default;
      ~ServiceThread() = default;

      bool Start(){
        return ThreadStart(&thread_, Service::GetName(), &HandleThread, (uword)0);
      }

      bool Join(){
        return ThreadJoin(thread_);
      }
    };
  }
}

#endif//TOKEN_HTTP_SERVICE_H