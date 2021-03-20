#ifndef TOKEN_TEST_WALLET_H
#define TOKEN_TEST_WALLET_H

#include "wallet.h"
#include "test_suite.h"
#include "mock/mock_wallet_manager.h"

namespace token{
  class WalletTest : public ::testing::Test{
   protected:
    WalletTest() = default;
   public:
    ~WalletTest() = default;
  };

  class WalletManagerTest : public ::testing::Test{
   protected:
    WalletManagerTest() = default;

    static void SetUpTestSuite(){
      LOG(INFO) << "setting up wallet manager test suite....";
      char tmp_filename[] = "/tmp/tkn-test-wallet-manager.XXXXXX";
      char* tmp_directory = mkdtemp(tmp_filename);
      ASSERT_TRUE(tmp_directory != NULL);

      std::string test_directory(tmp_directory);
      if(!FileExists(test_directory))
        ASSERT_TRUE(CreateDirectory(test_directory));
      LOG(INFO) << "using test directory: " << test_directory;

      ASSERT_TRUE(WalletManager::Initialize(test_directory));
    }

    static void TearDownTestSuite(){
      LOG(INFO) << "tearing down wallet manager test suite....";
    }
   public:
    ~WalletManagerTest() = default;
  };
}

#endif//TOKEN_TEST_WALLET_H