#include <gtest/gtest.h>
#include <glog/logging.h>

#include "helpers.h"
#include "token/hash.h"

namespace token{
 TEST(SHA256Test, TestHexString){
   static constexpr const char* kHexString = "B6FF77862B24E765056EC25E59749A6E834ACF74E393EFD7D7F4F8B219181531";
   auto h1 = sha256::FromHex(kHexString);
   ASSERT_EQ(h1.HexString(), std::string(kHexString));
 }

 TEST(SHA256Test, TestFromHex){
   static constexpr const char* kHexString = "B6FF77862B24E765056EC25E59749A6E834ACF74E393EFD7D7F4F8B219181531";
   ASSERT_EQ(sha256::FromHex(kHexString), sha256::FromHex(kHexString));
 }

 TEST(SHA256Test, TestConcat){
   static constexpr const char* kHexString = "B6FF77862B24E765056EC25E59749A6E834ACF74E393EFD7D7F4F8B219181531";
   uint256 h1 = sha256::FromHex(kHexString);
   uint256 h2 = sha256::FromHex(kHexString);
   uint256 h3 = sha256::Concat(h1, h2);
   ASSERT_EQ(h3.HexString(), "408AD6E74422A9581F48BAA6DA698D04B4BD419480EFA1F2713946CE1E8E7605");
 }
}