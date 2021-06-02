#include "http/test_http_controller_health.h"

namespace token{
  namespace http{
    TEST_F(HealthControllerTest, TestGetLiveStatus){
      SessionPtr session = NewMockHttpSession();
      ::testing::Mock::AllowLeak(session.get()); // not a leak, this is a shared ptr

      HealthController controller;

      RequestPtr request = NewGetRequest(session, "/status/live");
      ResponsePtr response = NewOkResponse(session, "Ok");

      EXPECT_CALL((MockHttpSession&)*session, Send(ResponseEquals(response)))
        .Times(testing::AtLeast(1));
      controller.OnGetLiveStatus(session, request);
    }

    TEST_F(HealthControllerTest, TestGetReadyStatus){
      SessionPtr session = NewMockHttpSession();
      ::testing::Mock::AllowLeak(session.get()); // not a leak, this is a shared ptr

      HealthController controller;

      RequestPtr request = NewGetRequest(session, "/status/ready");
      ResponsePtr response = NewOkResponse(session, "Ok");

      EXPECT_CALL((MockHttpSession&)*session, Send(ResponseEquals(response)))
          .Times(testing::AtLeast(1));
      controller.OnGetReadyStatus(session, request);
    }
  }
}