#ifndef TOKEN_HTTP_SERVICE_H
#define TOKEN_HTTP_SERVICE_H

#include "server.h"
#include "http/http_router.h"
#include "http/http_message.h"
#include "http/http_session.h"

namespace token{
  namespace http{
    class ServiceBase : public ServerBase<Message>{
     protected:
      RouterPtr router_;

      ServiceBase(uv_loop_t* loop, const char* name):
        ServerBase<Message>(loop, name),
        router_(Router::NewInstance()){}

      Session* CreateSession() const override{
        return new Session(GetLoop(), GetRouter());
      }
     public:
      ~ServiceBase() override = default;

      ServerPort GetPort() const override{
        return FLAGS_service_port;
      }

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
        DLOG_THREAD_IF(ERROR, !instance->Run()) << "Failed to run service loop.";
        pthread_exit(nullptr);
      }
     public:
      ServiceThread() = default;
      ~ServiceThread() = default;

      bool Start(){
        return ThreadStart(&thread_, Service::GetThreadName(), &HandleThread, (uword)0);
      }

      bool Join(){
        return ThreadJoin(thread_);
      }
    };
  }
}

#endif//TOKEN_HTTP_SERVICE_H