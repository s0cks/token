#include "http/test_http_controller_health.h"

namespace token{
  static inline HttpRequestPtr
  CreateHealthStatusRequest(MockHttpSession* session, const std::string& status){
    std::string path = "/status/" + status;
    ParameterMap params;
    return CreateRequestFor(session, HttpMethod::kGet, path, params);
  }

  static inline HttpRequestPtr
  CreateGetLiveStatusRequest(MockHttpSession* session){
    return CreateHealthStatusRequest(session, "live");
  }

  static inline HttpRequestPtr
  CreateGetReadyStatusRequest(MockHttpSession* session){
    return CreateHealthStatusRequest(session, "ready");
  }

  TEST_F(HealthControllerTest, TestGetLiveStatus){
    MockHttpSession session;
    EXPECT_CALL(session, Send(ResponseIs(HttpStatusCode::kHttpOk, "{\"data\":\"Ok\"}")))
      .Times(testing::AtLeast(1));
    controller_.OnGetLiveStatus(&session, CreateGetLiveStatusRequest(&session));
  }

  TEST_F(HealthControllerTest, TestGetReadyStatus){
    MockHttpSession session;
    EXPECT_CALL(session, Send(ResponseIs(HttpStatusCode::kHttpOk, "{\"data\":\"Ok\"}")))
      .Times(testing::AtLeast(1));
    controller_.OnGetReadyStatus(&session, CreateGetReadyStatusRequest(&session));
  }
}