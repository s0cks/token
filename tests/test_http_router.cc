#include "test_suite.h"
#include "http/router.h"

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

    TEST(TestHttpRouter, test_params){
        HttpRouter* router = new HttpRouter();
        router->Put("/account/:id", NULL);

        std::string request_body = HttpPutRequestBody("/account/TestUser");
        HttpRequestPtr request = std::make_shared<HttpRequest>(nullptr, request_body.data(), request_body.length());

        HttpRouterMatch match = router->Find(request);
        LOG(INFO) << "parameters (" << match.GetParameters().size() << ")";
        for(auto& it : match.GetParameters())
          LOG(INFO) << it.first << " := " << it.second;

        LOG(INFO) << "status: " << match.GetStatus();
        ASSERT_TRUE(match.IsOk());
        ASSERT_EQ(match.GetParameterValue("id"), "TestUser");
    }
}