#ifndef TOKEN_HASH_H
#define TOKEN_HASH_H

#include <set>
#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <sstream>
#include <unordered_set>
#include <leveldb/slice.h>

#include "token/crypto.h"

#ifdef TOKEN_JSON_EXPORT
#include "token/json.h"
#endif//TOKEN_JSON_EXPORT

namespace token{
 template<const uint64_t HashSize>
 class BigNumber{
  protected:
   uint8_t data_[HashSize];

   BigNumber():
    data_(){
     SetNull();
   }
   constexpr BigNumber(const uint8_t* data, size_t length):
    data_(){
     memcpy(data_, data, std::min(length, HashSize));
   }
  public:
   virtual ~BigNumber() = default;

   void SetNull(){
     memset(data_, 0, sizeof(data_));
   }

   bool IsNull() const{
     for(int64_t idx = 0; idx < HashSize; idx++){
       if(data_[idx] != 0){
         return false;
       }
     }
     return true;
   }

   const uint8_t* data() const{
     return data_;
   }

   uint64_t size() const{
     return HashSize;
   }

   std::string HexString() const{
     std::string hash;
     CryptoPP::ArraySource source(data(), size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash)));
     return hash;
   }

   std::string BinaryString() const{
     std::stringstream ss;
     std::bitset<256> bits;
     for(int idx = 0; idx < HashSize; idx++){
       uint8_t curr = data_[idx];
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

#define UINT256_SIZE (256 / 8)

 class uint256 : public BigNumber<UINT256_SIZE>{
  public:
   static constexpr const uint64_t kSize = UINT256_SIZE;

   typedef CryptoPP::SHA256 HashFunction;

   static inline int
   Compare(const uint256& lhs, const uint256& rhs){
     return memcmp(lhs.data(), rhs.data(), HashFunction ::DIGESTSIZE);
   }
  public:
   uint256() = default;
   constexpr uint256(const uint8_t* data, uint64_t size):
     BigNumber(data, size){
   }
   explicit uint256(const CryptoPP::SecByteBlock& blk):
     BigNumber(blk.data(), blk.size()){
   }
   uint256(const uint256& rhs) = default;
   ~uint256() override = default;

   uint256& operator=(const uint256& rhs) = default;

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

 class sha256{
  public:
   typedef CryptoPP::SHA256 Function;
   typedef uint256 Storage;

   static const uint64_t kDigestSize;
   static const uint64_t kSize;

   static inline uint256
   Of(const uint8_t* data, size_t length){
     Function function;
     CryptoPP::SecByteBlock digest(Function::DIGESTSIZE);
     CryptoPP::ArraySource source(data, length, true, new CryptoPP::HashFilter(function, new CryptoPP::ArraySink(digest.data(), digest.size())));
     return uint256(digest);
   }

   static inline uint256
   FromHex(const std::string& val){
     CryptoPP::SecByteBlock digest(Function::DIGESTSIZE);
     CryptoPP::StringSource source(val, true, new CryptoPP::HexDecoder(new CryptoPP::ArraySink(digest.data(), digest.size())));
     return uint256(digest);
   }

   static inline uint256
   Concat(const uint256& lhs, const uint256& rhs){
     CryptoPP::SecByteBlock data(lhs.size() + rhs.size());
     memcpy(&data[0], lhs.data(), lhs.size());
     memcpy(&data[lhs.size()], rhs.data(), rhs.size());

     CryptoPP::SecByteBlock digest(Function::DIGESTSIZE);

     Function function;
     CryptoPP::ArraySource source(data.data(), data.size(), true, new CryptoPP::HashFilter(function, new CryptoPP::ArraySink(digest.data(), digest.size())));
     return uint256(digest);
   }

   static inline uint256
   Nonce(uint64_t size = 4096){
     CryptoPP::SecByteBlock nonce(size);
     CryptoPP::SecByteBlock digest(Function::DIGESTSIZE);

     CryptoPP::AutoSeededRandomPool prng;
     prng.GenerateBlock(nonce, size);

     CryptoPP::ArraySource source(nonce, nonce.size(), true, new CryptoPP::HexEncoder(new CryptoPP::ArraySink(digest.data(), digest.size())));
     return uint256(digest);
   }
 };

// typedef std::vector<Hash> HashList;
// typedef std::set<Hash, Hash::Comparator> HashSet;
//
// static inline HashList&
// operator<<(HashList& list, const Hash& val){
//   list.push_back(val);
//   return list;
// }
//
// template<class T>
// static inline HashList&
// operator<<(HashList& list, const std::shared_ptr<T>& val){
//   list.push_back(val->GetHash());
//   return list;
// }

#ifdef TOKEN_JSON_EXPORT
 namespace json{
   static inline bool
   SetField(Writer& writer, const char* name, const Hash& val){
     JSON_KEY(writer, name);
     JSON_STRING(writer, val.HexString());
     return true;
   }

   static inline bool
   Write(Writer& writer, const HashList& val){
     JSON_START_ARRAY(writer);
     {
       for(auto& it : val)
         JSON_STRING(writer, it.HexString());
     }
     JSON_END_ARRAY(writer);
     return true;
   }

   static inline bool
   SetField(Writer& writer, const char* name, const HashList& val){
     JSON_KEY(writer, name);
     return Write(writer, val);
   }
 }
#endif//TOKEN_JSON_EXPORT
}

#endif //TOKEN_HASH_H