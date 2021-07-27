#ifndef TOKEN_HTTP_SERVICE_H
#define TOKEN_HTTP_SERVICE_H

#include "server/server.h"
#include "http/http_router.h"
#include "http/http_message.h"
#include "http/http_session.h"

namespace token{
  namespace http{
    class ServiceBase;
    class ServiceSession : public Session{
    private:
      ServiceBase* service_;
    public:
      ServiceSession():
        Session(),
        service_(nullptr){}
      ServiceSession(uv_loop_t* loop, const UUID& session_id, ServiceBase* service):
        Session(loop, session_id),
        service_(service){}
      ServiceSession(uv_loop_t* loop, ServiceBase* service):
        ServiceSession(loop, UUID(), service){}
      explicit ServiceSession(ServiceBase* service);
      ~ServiceSession() override = default;

      ServiceBase* GetService() const{
        return service_;
      }

      void OnMessageRead(const internal::BufferPtr& buff) override;
    };

    class ServiceBase : public ServerBase<ServiceSession>{
     protected:
      Router router_;

      explicit ServiceBase(uv_loop_t* loop):
        ServerBase(loop),
        router_(){}

      ServiceSession* CreateSession() override{
        return new ServiceSession(this);
      }
     public:
      ~ServiceBase() override = default;

      Router& GetRouter(){
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

    class AsyncRequestHandler{
    private:
      uv_async_t request_;
      ServiceSession* session_;
      internal::BufferPtr buffer_;//TODO: remove

      inline ServiceSession*
      session() const{
        return session_;
      }

      RequestPtr ParseRequest();
      RouterMatch FindMatch(const RequestPtr& request);

      static void DoWork(uv_async_t* request);
    public:
      AsyncRequestHandler(uv_loop_t* loop, ServiceSession* session, const internal::BufferPtr& buff):
          request_(),
          session_(session),
          buffer_(buff){
        request_.data = this;
        CHECK_UVRESULT2(ERROR, uv_async_init(loop, &request_, &DoWork), "couldn't initialize async handle.");
      }
      ~AsyncRequestHandler() = default;

      bool Execute(){
        VERIFY_UVRESULT2(ERROR, uv_async_send(&request_), "couldn't send async request.");
        return true;
      }
    };
  }
}

#endif//TOKEN_HTTP_SERVICE_H