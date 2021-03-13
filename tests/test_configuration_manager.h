#ifndef TOKEN_TEST_CONFIGURATION_MANAGER_H
#define TOKEN_TEST_CONFIGURATION_MANAGER_H

#include "test_suite.h"
#include "configuration.h"

namespace token{
  class ConfigurationManagerIntegrationTest : public IntegrationTest{
   protected:
    ConfigurationManagerIntegrationTest():
      IntegrationTest(){}

    static void SetUpTestSuite();
    static void TearDownTestSuite();
   public:
    virtual ~ConfigurationManagerIntegrationTest() = default;
  };
}

#endif//TOKEN_TEST_CONFIGURATION_MANAGER_H