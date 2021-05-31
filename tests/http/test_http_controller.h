#ifndef TOKEN_TEST_HTTP_CONTROLLER_H
#define TOKEN_TEST_HTTP_CONTROLLER_H

#include "test_suite.h"
#include "mock/mock_http_session.h"

namespace token{
  MATCHER(ResponseIsOk, "The http response status code is 200 (Ok)"){
    return std::static_pointer_cast<http::Response>(arg)->GetStatusCode() == http::StatusCode::kHttpOk;
  }

  MATCHER_P(ResponseBodyEqualsString, expected, "The http response body doesn't match"){
    return std::static_pointer_cast<http::Response>(arg)->GetBodyAsString() == expected;
  }

  static inline http::RequestPtr
  CreateRequestFor(const http::SessionPtr& session, const http::Method& method, const std::string& path, const http::ParameterMap& path_params){
    http::RequestBuilder builder(session);
    builder.SetMethod(method);
    builder.SetPath(path);
    builder.SetPathParameters(path_params);
    return builder.Build();
  }

  namespace http{
    template<class Controller>
    class ControllerTest : public ::testing::Test{
     protected:
      std::shared_ptr<Controller> controller_;

      ControllerTest():
        ::testing::Test(),
        controller_(Controller::NewInstance()){}
      ControllerTest(const std::shared_ptr<Controller>& controller):
        ::testing::Test(),
        controller_(controller){}
     public:
      virtual ~ControllerTest() override = default;

      std::shared_ptr<Controller> GetController() const{
        return controller_;
      }
    };
  }
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_H