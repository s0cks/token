#include <gtest/gtest.h>
#include <glog/logging.h>

#include "helpers.h"
#include "token/block.h"

namespace token{
 class BlockTest : public ::testing::Test{
  private:
   static const Timestamp kTimestamp;

   Block* block_;
  protected:
   void SetUp() override{
     block_ = new Block(10, Clock::now(), transactions, 2);
   }
  public:
   BlockTest() = default;
   ~BlockTest() override = default;
 };

 TEST_F(BlockTest, TestEquality){

 }
}