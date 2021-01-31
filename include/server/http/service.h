#ifndef TOKEN_HTTP_SERVICE_H
#define TOKEN_HTTP_SERVICE_H

#include "common.h"
#include "server/server.h"
#include "server/http/router.h"

namespace Token{
  class HttpMessage;
  typedef std::shared_ptr<HttpMessage> HttpMessagePtr;

  class HttpSession;
  class HttpMessage : public Message{
   protected:
    HttpMessage() = default;
   public:
    virtual ~HttpMessage() = default;

    static HttpMessagePtr From(HttpSession* session, const BufferPtr& buffer);
  };

  class HttpSession : public Session<HttpMessage>{
    friend class Server<HttpMessage, HttpSession>;
   private:
    typedef SessionWriteData<HttpSession> HttpSessionWriteData;
   private:
    void OnMessageRead(const HttpMessagePtr& msg);
   public:
    HttpSession(uv_loop_t* loop):
      Session(loop){
      handle_.data = this;
    }
    ~HttpSession() = default;

    void Send(const HttpMessagePtr& msg){
      return SendMessages({ msg });
    }
  };

  class HttpService : protected Server<HttpMessage, HttpSession>{
   protected:
    HttpRouter router_;

    HttpService(uv_loop_t* loop):
        Server(loop, "http/service"),
        router_(){}
   public:
    virtual ~HttpService() = default;

    ServerPort GetPort() const{
      return FLAGS_service_port;
    }

    HttpRouter* GetRouter(){
      return &router_;
    }
  };

  class HttpRestService : HttpService{
   public:
    HttpRestService(uv_loop_t* loop=uv_loop_new());
    ~HttpRestService() = default;

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

#endif//TOKEN_HTTP_SERVICE_H