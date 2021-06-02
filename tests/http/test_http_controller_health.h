#ifndef TOKEN_TEST_HTTP_CONTROLLER_HEALTH_H
#define TOKEN_TEST_HTTP_CONTROLLER_HEALTH_H

#include "http/test_http_controller.h"
#include "http/http_controller_health.h"

namespace token{
  namespace http{
   class HealthControllerTest : public ::testing::Test{
    public:
     HealthControllerTest() = default;
     ~HealthControllerTest() override = default;
   };
  }
}

#endif//TOKEN_TEST_HTTP_CONTROLLER_HEALTH_H