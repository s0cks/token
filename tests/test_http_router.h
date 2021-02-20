#ifndef TOKEN_TEST_HTTP_ROUTER_H
#define TOKEN_TEST_HTTP_ROUTER_H

#include "test_suite.h"
#include "http/http_router.h"
#include "http/http_request.h"
#include "http/http_response.h"

namespace token{
#define HTTP_PUT(Path, Handler) \
  router_.Put((Path), (Handler))
#define HTTP_GET(Path, Handler) \
  router_.Get((Path), (Handler))
#define HTTP_DELETE(Path, Handler) \
  router_.Delete((Path), (Handler))
#define HTTP_POST(Path, Handler) \
  router_.Post((Path), (Handler))

  class HttpRouterTest : public ::testing::Test{
   protected:
    HttpRouter router_;

    HttpRouterTest():
      ::testing::Test(),
      router_(){}

    void SetUp(){
      HTTP_PUT("/hello/world", NULL);
    }

    void TearDown(){}

    HttpRouterMatch FindMatch(const std::string& request){
      HttpRequestPtr req = std::make_shared<HttpRequest>(nullptr, request.data(), request.length());
      return router_.Find(req);
    }

    static inline std::string
    HttpGetRequestBody(const std::string& path){
      return "GET " + path + " HTTP/1.1";
    }

    static inline std::string
    HttpPutRequestBody(const std::string& path){
      return "PUT " + path + " HTTP/1.1";
    }

    static inline std::string
    HttpPostRequestBody(const std::string& path){
      return "POST " + path + " HTTP/1.1";
    }

    static inline std::string
    HttpDeleteRequestBody(const std::string& path){
      return "DELETE " + path + " HTTP/1.1";
    }
   public:
    ~HttpRouterTest() = default;
  };
#undef HTTP_PUT
#undef HTTP_GET
#undef HTTP_POST
#undef HTTP_DELETE
}

#endif//TOKEN_TEST_HTTP_ROUTER_H