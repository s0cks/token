#include "test_blockchain.h"

#include "pool.h"
#include "wallet.h"
#include "configuration.h"
#include "job/scheduler.h"

namespace token{
  void BlockChainTest::SetUpTestSuite(){
    char tmp_filename[] = "/tmp/tkn-bchain.XXXXXX";
    char* tmp_directory = mkdtemp(tmp_filename);
    ASSERT_TRUE(tmp_directory != NULL);

    std::string test_directory = std::string(tmp_directory);
    if(!FileExists(test_directory))
      ASSERT_TRUE(CreateDirectory(test_directory));
    LOG(INFO) << "using test directory: " << test_directory;

    ASSERT_TRUE(JobScheduler::Initialize());
    ASSERT_TRUE(ConfigurationManager::Initialize(test_directory + "/config"));
    ASSERT_TRUE(ConfigurationManager::SetProperty(TOKEN_CONFIGURATION_BLOCKCHAIN_HOME, test_directory));

    ASSERT_TRUE(WalletManager::Initialize());
    ASSERT_TRUE(ObjectPool::Initialize());
    ASSERT_TRUE(BlockChain::Initialize());
  }

  TEST_F(BlockChainTest, TestHasBlock){
    ASSERT_TRUE(BlockChain::HasBlock(BlockChain::GetReference(BLOCKCHAIN_REFERENCE_GENESIS)));
    ASSERT_TRUE(BlockChain::HasBlock(BlockChain::GetReference(BLOCKCHAIN_REFERENCE_HEAD)));
    ASSERT_FALSE(BlockChain::HasBlock(Hash::GenerateNonce()));
    ASSERT_FALSE(BlockChain::HasBlock(Hash::GenerateNonce()));
  }

  TEST_F(BlockChainTest, TestPutGetReference){
    std::string reference_name = "Test";
    Hash hash = Hash::GenerateNonce();
    ASSERT_TRUE(BlockChain::PutReference(reference_name, hash));
    ASSERT_TRUE(BlockChain::HasReference(reference_name));
  }
}