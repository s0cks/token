#ifndef TOKEN_BIGNUMBER_H
#define TOKEN_BIGNUMBER_H

#include <bitset>
#include <string>
#include <sstream>
#include <algorithm>
#include "token/platform.h"

namespace token{
 template<int64_t NumberOfBits>
 class BigNumber{
  public:
   static constexpr const int64_t kNumberOfBits = NumberOfBits;
   static constexpr const int64_t kNumberOfBytes = kNumberOfBits * kBitsPerByte;
   static constexpr const int64_t kSize = kNumberOfBytes;
  private:
   static constexpr const char* kHexDigits = "0123456789ABCDEF";
  protected:
   uint8_t data_[kSize];

   BigNumber():
    data_(){
     reset();
   }

   BigNumber(const uint8_t* data, int64_t size):
    data_(){
     reset();
     copy_from(data, size);
   }

   void copy_from(const uint8_t* data, int64_t size = kSize){
     std::memcpy(data_, data, size);
   }

   void reset(){
     std::memset(data_, 0, kSize);
   }
  public:
   virtual ~BigNumber() = default;

   virtual const uint8_t* data() const{
     return data_;
   }

   virtual int64_t size() const{
     return kSize;
   }

   const uint8_t* const_begin() const{
     return data();
   }

   const uint8_t* const_end() const{
     return data() + size();
   }

   std::string HexString() const{
     std::string result;
     result.reserve(kSize * 2);
     std::for_each(const_begin(), const_end(), [&result](uint8_t inputByte){
       result.push_back(kHexDigits[inputByte >> 4]);
       result.push_back(kHexDigits[inputByte & 0x0F]);
     });
     return result;
   }

   std::string BinaryString() const{ //TODO: optimize
     std::stringstream ss;
     std::bitset<256> bits;
     for(int idx = 0; idx < size(); idx++){
       auto curr = data()[idx];
       int offset = idx * CHAR_BIT;
       for(int bit = 0; bit < CHAR_BIT; bit++){
         bits[offset] = curr & 1;
         offset++;
         curr >>= 1;
       }
     }
     ss << bits;
     return ss.str();
   }
 };
}

#endif//TOKEN_BIGNUMBER_H