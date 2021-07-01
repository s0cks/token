#ifndef TOKEN_TEST_HTTP_CONTROLLER_H
#define TOKEN_TEST_HTTP_CONTROLLER_H

#include "test_suite.h"
#include "mock_http_session.h"

namespace token{
  MATCHER_P(ResponseEquals, expected, ""){
    http::ResponsePtr actual = std::static_pointer_cast<http::Response>(arg);
    return actual->GetStatusCode() == expected->GetStatusCode()
        && actual->GetBodyAsString() == expected->GetBodyAsString();
  }

  MATCHER(ResponseIsOk, "The http response status code is 200 (Ok)"){
    return std::static_pointer_cast<http::Response>(arg)->GetStatusCode() == http::StatusCode::kOk;
  }

  MATCHER_P(ResponseBodyEqualsString, expected, "The http response body doesn't match"){
    DLOG(INFO) << "asserting that " << std::static_pointer_cast<http::Response>(arg)->GetBodyAsString() << " equals: " << expected;
    return std::static_pointer_cast<http::Response>(arg)->GetBodyAsString() == expected;
  }

  static inline http::RequestPtr
  CreateRequestFor(const http::Method& method, const std::string& path, const http::ParameterMap& path_params){
    http::RequestBuilder builder;
    builder.SetMethod(method);
    builder.SetPath(path);
    builder.SetPathParameters(path_params);
    return builder.Build();
  }
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_H