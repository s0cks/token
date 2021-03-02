#include "test_blockchain.h"

namespace token{
  static std::default_random_engine engine;

  static inline int64_t
  GenerateRandomSize(){
    static std::uniform_int_distribution<int> distribution(0, 65536);
    return (int64_t)distribution(engine);
  }

  static inline BlockChain::BlockKey
  NewBlockKey(const Hash& hash=Hash::GenerateNonce(), const int64_t& size=GenerateRandomSize()){
    return BlockChain::BlockKey(size, hash);
  }

  static inline BlockChain::ReferenceKey
  NewReferenceKey(const std::string& reference){
    return BlockChain::ReferenceKey(reference);
  }

  TEST_F(BlockChainTest, test_blk_key){
    BlockChain::BlockKey k1 = NewBlockKey();
    BlockChain::BlockKey k2 = NewBlockKey();
    ASSERT_EQ(k1, k1);
    ASSERT_EQ(k2, k2);
    ASSERT_NE(k1, k2);
  }

  TEST_F(BlockChainTest, test_comparator){
    BlockChain::Comparator comparator;
    BlockChain::BlockKey k1 = NewBlockKey();
    BlockChain::BlockKey k2 = NewBlockKey();
    BlockChain::ReferenceKey k3 = NewReferenceKey("<HEAD>");

    ASSERT_EQ(comparator.Compare(k1, k2), -1);
    ASSERT_EQ(comparator.Compare(k2, k1), +1);
    ASSERT_EQ(comparator.Compare(k3, k2), +1);
    ASSERT_EQ(comparator.Compare(k3, k1), +1);
    ASSERT_EQ(comparator.Compare(k2, k3), -1);
    ASSERT_EQ(comparator.Compare(k1, k3), -1);
  }
}