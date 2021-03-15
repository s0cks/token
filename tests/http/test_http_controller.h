#ifndef TOKEN_TEST_HTTP_CONTROLLER_H
#define TOKEN_TEST_HTTP_CONTROLLER_H

#include "test_suite.h"
#include "mocks/mock_http_session.h"

namespace token{
  static inline HttpRequestPtr
  CreateRequestFor(MockHttpSession* session, const HttpMethod& method, const std::string& path, const ParameterMap& path_params){
    HttpRequestBuilder builder(session);
    builder.SetMethod(method);
    builder.SetPath(path);
    builder.SetPathParameters(path_params);
    return builder.Build();
  }

  MATCHER_P2(ResponseIs, status_code, body, "response doesn't match"){
    HttpResponsePtr a = std::static_pointer_cast<HttpResponse>(arg);
    return a->GetStatusCode() == status_code
        && a->GetBodyAsString() == body;
  }

  class HttpControllerTest : public ::testing::Test{
   protected:
    HttpControllerTest():
      ::testing::Test(){}
   public:
    virtual ~HttpControllerTest() = default;
  };
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_H