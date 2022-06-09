#ifndef TOKEN_UINT256_H
#define TOKEN_UINT256_H

#include "token/bignumber.h"

namespace token{
 static constexpr const int64_t kNumberOfBitsForUInt256 = 256;

#define UINT256_SIZE (kNumberOfBitsForUInt256 / kBitsPerByte)

 class uint256 : public BigNumber<UINT256_SIZE>{
  public:
   static inline int
   Compare(const uint256& lhs, const uint256& rhs){
     return memcmp(lhs.data(), rhs.data(), kSize);
   }
  public:
   uint256() = default;
   uint256(const uint8_t* data, int64_t size):
     BigNumber(data, size){
   }
   uint256(const uint256& rhs) = default;
   ~uint256() override = default;

   uint256& operator=(const uint256& rhs) = default;

   uint8_t& operator[](uint64_t index){
     return data_[index];
   }

   uint8_t operator[](uint64_t index) const{
     return data_[index];
   }

   friend std::ostream& operator<<(std::ostream& stream, const uint256& val){
     return stream << val.HexString();
   }

   friend bool operator==(const uint256& lhs, const uint256& rhs){
     return Compare(lhs, rhs) == 0;
   }

   friend bool operator!=(const uint256& lhs, const uint256& rhs){
     return Compare(lhs, rhs) != 0;
   }

   friend bool operator<(const uint256& lhs, const uint256& rhs){
     return Compare(lhs, rhs) < 0;
   }

   friend bool operator>(const uint256& lhs, const uint256& rhs){
     return Compare(lhs, rhs) > 0;
   }
 };
}

#endif//TOKEN_UINT256_H