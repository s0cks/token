#ifndef TOKEN_MOCK_CONFIGURATION_MANAGER_H
#define TOKEN_MOCK_CONFIGURATION_MANAGER_H

namespace token{
  class MockConfigurationManager : public ConfigurationManager{
   public:
    MockConfigurationManager() = default;
    ~MockConfigurationManager() override = default;
  };
}

#endif//TOKEN_MOCK_CONFIGURATION_MANAGER_H