#include "common.h"
#include "test_configuration_manager.h"

namespace token{
  static const UUID testProperty1 = UUID();
  static const NodeAddress testProperty2 = NodeAddress("127.0.0.1:8080");
  static const std::string testProperty3 = "Hello World";

#define TEST_PROPERTY_1 "testProperty1"
#define TEST_PROPERTY_2 "testProperty2"
#define TEST_PROPERTY_3 "testProperty3"

  void ConfigurationManagerIntegrationTest::SetUpTestSuite(){
    std::string test_directory = GenerateRandomTestDirectory("tkn-config");
    ASSERT_FALSE(test_directory.empty());
    ASSERT_TRUE(FileExists(test_directory));
    ASSERT_TRUE(ConfigurationManager::Initialize(test_directory));

    ConfigurationManager* instance = ConfigurationManager::GetInstance();
    ASSERT_TRUE(instance->PutProperty(TEST_PROPERTY_1, testProperty1));
    ASSERT_TRUE(instance->PutProperty(TEST_PROPERTY_2, testProperty2));
    ASSERT_TRUE(instance->PutProperty(TEST_PROPERTY_3, testProperty3));
  }

  void ConfigurationManagerIntegrationTest::TearDownTestSuite(){
    //TODO: cleanup test_directory
  }

  TEST_F(ConfigurationManagerIntegrationTest, TestHasProperty){
    ConfigurationManager* instance = ConfigurationManager::GetInstance();
    ASSERT_TRUE(instance->HasProperty(TEST_PROPERTY_1));
    ASSERT_TRUE(instance->HasProperty(TEST_PROPERTY_2));
  }

  TEST_F(ConfigurationManagerIntegrationTest, TestGetUUID){
    ConfigurationManager* instance = ConfigurationManager::GetInstance();
    UUID actual;
    ASSERT_TRUE(instance->GetUUID(TEST_PROPERTY_1, actual));
    ASSERT_EQ(actual, testProperty1);
  }

  TEST_F(ConfigurationManagerIntegrationTest, TestGetPeerList){
    ConfigurationManager* instance = ConfigurationManager::GetInstance();
    PeerList actual;
    ASSERT_TRUE(instance->GetPeerList(TEST_PROPERTY_2, actual));
    PeerList expected;
    expected.insert(testProperty2);
    ASSERT_EQ(actual, expected);
  }

  TEST_F(ConfigurationManagerIntegrationTest, TestGetString){
    ConfigurationManager* instance = ConfigurationManager::GetInstance();
    std::string actual;
    ASSERT_TRUE(instance->GetString(TEST_PROPERTY_3, actual));
    ASSERT_EQ(actual, testProperty3);
  }
}