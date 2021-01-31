#include "test_suite.h"
#include "utils/buffer.h"
#include "server/http/router.h"
#include "server/http/request.h"

namespace Token{
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

  TEST(TestHttpRouter, test_pos){
    HttpRouter* router = new HttpRouter();
    router->Get("/hello/world", NULL);

    std::string request_body = HttpGetRequestBody("/hello/world");
    HttpRequestPtr request = std::make_shared<HttpRequest>(nullptr, request_body.data(), request_body.length());

    HttpRouterMatch match = router->Find(request);
    ASSERT_TRUE(match.IsOk());
  }

  TEST(TestHttpRouter, test_neg){
    HttpRouter* router = new HttpRouter();
    router->Get("/hello/world", NULL);

    std::string request_body = HttpDeleteRequestBody("/hello/world");
    HttpRequestPtr request = std::make_shared<HttpRequest>(nullptr, request_body.data(), request_body.length());

    HttpRouterMatch match = router->Find(request);
    ASSERT_TRUE(match.IsNotSupported());
  }

  TEST(TestHttpRouter, test_path_params){
    HttpRouter* router = new HttpRouter();
    router->Put("/account/:id", NULL);

    std::string request_body = HttpPutRequestBody("/account/TestUser");
    HttpRequestPtr request = std::make_shared<HttpRequest>(nullptr, request_body.data(), request_body.length());

    HttpRouterMatch match = router->Find(request);
    LOG(INFO) << "match: " << match;
    LOG(INFO) << "query parameters:";
    for(auto& it : match.GetQueryParameters()){
      LOG(INFO) << " - " << it.first << " := " << it.second;
    }

    LOG(INFO) << "path parameters:";
    for(auto& it : match.GetPathParameters()){
      LOG(INFO) << " - " << it.first << " := " << it.second;
    }

    ASSERT_TRUE(match.IsOk());
    ASSERT_EQ(match.GetPathParameterValue("id"), "TestUser");
  }

  TEST(TestHttpRouter, test_path_params2){
    HttpRouter* router = new HttpRouter();
    router->Put("/wallet/:user/tokens/:hash", NULL);

    std::string request_body = HttpPutRequestBody("/wallet/TestUser/tokens/test-token");
    HttpRequestPtr request = std::make_shared<HttpRequest>(nullptr, request_body.data(), request_body.length());

    HttpRouterMatch match = router->Find(request);
    LOG(INFO) << "match: " << match;
    LOG(INFO) << "query parameters:";
    for(auto& it : match.GetQueryParameters()){
      LOG(INFO) << " - " << it.first << " := " << it.second;
    }

    LOG(INFO) << "path parameters:";
    for(auto& it : match.GetPathParameters()){
      LOG(INFO) << " - " << it.first << " := " << it.second;
    }

    ASSERT_TRUE(match.IsOk());
    ASSERT_EQ(match.GetPathParameterValue("user"), "TestUser");
  }

  TEST(TestHttpRouter, test_query_params){
    HttpRouter* router = new HttpRouter();
    router->Get("/wallet/:user", NULL);

    std::string request_body = HttpGetRequestBody("/wallet/VenueA?scale=64");
    HttpRequestPtr request = std::make_shared<HttpRequest>(nullptr, request_body.data(), request_body.length());

    HttpRouterMatch match = router->Find(request);
    LOG(INFO) << "match: " << match;
    LOG(INFO) << "query parameters:";
    for(auto& it : match.GetQueryParameters()){
      LOG(INFO) << " - " << it.first << " := " << it.second;
    }

    LOG(INFO) << "path parameters:";
    for(auto& it : match.GetPathParameters()){
      LOG(INFO) << " - " << it.first << " := " << it.second;
    }

    ASSERT_TRUE(match.IsOk());
    ASSERT_EQ(match.GetPathParameterValue("user"), "VenueA");
    ASSERT_TRUE(match.HasQueryParameter("scale"));
    ASSERT_EQ(match.GetQueryParameterValue("scale"), "64");
  }

  TEST(TestHttpRouter, test_query_params2){
    HttpRouter* router = new HttpRouter();
    router->Get("/wallet/:user/tokens/:hash", NULL);

    std::string request_body = HttpGetRequestBody("/wallet/VenueA/tokens/test?scale=64&test=true");
    HttpRequestPtr request = std::make_shared<HttpRequest>(nullptr, request_body.data(), request_body.length());

    HttpRouterMatch match = router->Find(request);
    LOG(INFO) << "match: " << match;
    LOG(INFO) << "query parameters:";
    for(auto& it : match.GetQueryParameters()){
      LOG(INFO) << " - " << it.first << " := " << it.second;
    }

    LOG(INFO) << "path parameters:";
    for(auto& it : match.GetPathParameters()){
      LOG(INFO) << " - " << it.first << " := " << it.second;
    }

    ASSERT_TRUE(match.IsOk());
    ASSERT_EQ(match.GetPathParameterValue("user"), "VenueA");
    ASSERT_TRUE(match.HasQueryParameter("scale"));
    ASSERT_EQ(match.GetQueryParameterValue("scale"), "64");
    ASSERT_TRUE(match.HasQueryParameter("test"));
    ASSERT_EQ(match.GetQueryParameterValue("test"), "true");
  }
}