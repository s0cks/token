#include "http/test_http_controller_health.h"

namespace token{
  static inline http::RequestPtr
  CreateHealthStatusRequest(const http::SessionPtr& session, const std::string& path){
    return CreateRequestFor(session, http::Method::kGet, "/status/" + path, http::ParameterMap());
  }

  static inline http::RequestPtr
  CreateGetLiveStatusRequest(const http::SessionPtr& session){
    return CreateHealthStatusRequest(session, "live");
  }

  static inline http::RequestPtr
  CreateGetReadyStatusRequest(const http::SessionPtr& session){
    return CreateHealthStatusRequest(session, "ready");
  }

  TEST_F(HealthControllerTest, TestGetLiveStatus){
    http::SessionPtr session = NewMockHttpSession();

    EXPECT_CALL((MockHttpSession&)*session, Send(::testing::AllOf(ResponseIsOk(), ResponseBodyEqualsString("{\"data\":\"Ok\"}"))))
      .Times(testing::AtLeast(1));
    GetController()->OnGetLiveStatus(session, CreateGetLiveStatusRequest(session));
  }

  TEST_F(HealthControllerTest, TestGetReadyStatus){
    http::SessionPtr session = NewMockHttpSession();

    EXPECT_CALL((MockHttpSession&)*session, Send(::testing::AllOf(ResponseIsOk(), ResponseBodyEqualsString("{\"data\":\"Ok\"}"))))
      .Times(testing::AtLeast(1));
    GetController()->OnGetReadyStatus(session, CreateGetReadyStatusRequest(session));
  }
}