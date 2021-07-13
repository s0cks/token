#ifndef TOKEN_HTTP_SERVICE_H
#define TOKEN_HTTP_SERVICE_H

#include "server.h"
#include "http_router.h"
#include "http_message.h"
#include "http_session.h"

namespace token{
  namespace http{
    class ServiceBase : public ServerBase<http::Session>{
     protected:
      RouterPtr router_;

      explicit ServiceBase(uv_loop_t* loop):
        ServerBase(loop),
        router_(Router::NewInstance()){}

      http::Session* CreateSession() const override{
        return new Session(GetLoop(), GetRouter());
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
        DLOG_IF(ERROR, !instance->Run(Service::GetPort())) << "Failed to run the " << Service::GetName() << " service loop.";
        pthread_exit(nullptr);
      }
     public:
      ServiceThread() = default;
      ~ServiceThread() = default;

      bool Start(){
        return platform::ThreadStart(&thread_, Service::GetName(), &HandleThread, (uword)0);
      }

      bool Join(){
        return platform::ThreadJoin(thread_);
      }
    };
  }
}

#endif//TOKEN_HTTP_SERVICE_H