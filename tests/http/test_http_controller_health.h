#ifndef TOKEN_TEST_HTTP_CONTROLLER_HEALTH_H
#define TOKEN_TEST_HTTP_CONTROLLER_HEALTH_H

#include "http/test_http_controller.h"
#include "http/http_controller_health.h"

namespace token{
  class HealthControllerTest : public HttpControllerTest{
   protected:
    HealthController controller_;

   public:
    HealthControllerTest():
      HttpControllerTest(),
      controller_(){}
    ~HealthControllerTest() = default;
  };
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_HEALTH_H