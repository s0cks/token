#ifndef TOKEN_HASH_H
#define TOKEN_HASH_H

#include <bitset>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_set>
#include <leveldb/slice.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "common.h"

namespace Token{
  class Hash{
   public:
    static const int64_t kSize = 256 / 8;
   public:
    struct Hasher{
      size_t operator()(const Hash& hash) const{
        uint64_t a = ((uint64_t*) hash.data())[0];
        uint64_t b = ((uint64_t*) hash.data())[1];
        uint64_t c = ((uint64_t*) hash.data())[2];
        uint64_t d = ((uint64_t*) hash.data())[3];

        size_t res = 17;
        res = res * 31 + std::hash<uint64_t>()(a);
        res = res * 31 + std::hash<uint64_t>()(b);
        res = res * 31 + std::hash<uint64_t>()(c);
        res = res * 31 + std::hash<uint64_t>()(d);
        return res;
      }
    };

    struct Equal{
      bool operator()(const Hash& a, const Hash& b) const{
        return a == b;
      }
    };
   private:
    uint8_t data_[kSize];

    Hash(const unsigned char* data):
      data_(){
      memcpy(data_, data, sizeof(data_));
    }

    Hash(const std::string& hex):
      data_(){
      CryptoPP::StringSource source(hex, true, new CryptoPP::HexDecoder(new CryptoPP::ArraySink(data_, GetSize())));
    }

    Hash(const Hash& first, const Hash& second):
      data_(){
      size_t size = first.size() + second.size();
      uint8_t bytes[size];
      memcpy(&bytes[0], first.data(), first.size());
      memcpy(&bytes[first.size()], second.data(), second.size());
      CryptoPP::SHA256 func;
      CryptoPP::ArraySource
        source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(data_, GetSize())));
    }
   public:
    Hash():
      data_(){
      SetNull();
    }
    Hash(uint8_t* data, int64_t size = Hash::GetSize()):
      data_(){
      memcpy(data_, data, size);
    }
    Hash(const uint8_t* data, int64_t size = Hash::GetSize()):
      data_(){
      memcpy(data_, data, size);
    }
    Hash(const leveldb::Slice& slice, int64_t size = Hash::GetSize()):
      data_(){
      memcpy(data_, slice.data(), std::min((int64_t) slice.size(), size));
    }
    Hash(const Hash& hash):
      data_(){
      memcpy(data_, hash.data_, GetSize());
    }
    ~Hash() = default;

    void SetNull(){
      memset(data_, 0, sizeof(data_));
    }

    bool IsNull() const{
      for(int64_t idx = 0; idx < GetSize(); idx++){
        if(data_[idx] != 0){
          return false;
        }
      }
      return true;
    }

    unsigned char* data() const{
      return (unsigned char*) data_;
    }

    int64_t size() const{
      return kSize;
    }

    bool Encode(uint8_t* bytes, const int64_t& size) const{
      if(size < (int64_t) kSize){
        return false;
      }
      memcpy(bytes, data_, kSize);
      return true;
    }

    bool Encode(leveldb::Slice* slice) const{
      if(!slice){
        return false;
      }
      uint8_t bytes[kSize];
      Encode(bytes, kSize);
      (*slice) = leveldb::Slice((char*) bytes, kSize);
      return true;
    }

    std::string HexString() const{
      std::string hash;
      CryptoPP::ArraySource source(data(), size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash)));
      return hash;
    }

    std::string BinaryString() const{
      std::stringstream ss;
      std::bitset<256> bits;
      for(int idx = 0; idx < kSize; idx++){
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

    inline int Compare(const Hash& other) const{
      return memcmp(data_, other.data_, sizeof(data_));
    }

    friend inline bool operator==(const Hash& a, const Hash& b){
      return a.Compare(b) == 0;
    }

    friend inline bool operator!=(const Hash& a, const Hash& b){
      return a.Compare(b) != 0;
    }

    friend inline bool operator<(const Hash& a, const Hash& b){
      return a.Compare(b) < 0;
    }

    friend inline bool operator>(const Hash& a, const Hash& b){
      return a.Compare(b) > 0;
    }

    void operator=(const Hash& other){
      memcpy(data_, other.data_, kSize);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Hash& hash){
      stream << hash.HexString();
      return stream;
    }

    static inline int64_t
    GetSize(){
      return kSize;
    }

    static inline Hash
    FromHexString(const std::string& hex){
      return Hash(hex);
    }

    static inline Hash
    Concat(const Hash& first, const Hash& second){
      return Hash(first, second);
    }

    static inline Hash
    GenerateNonce(){
      CryptoPP::SecByteBlock nonce(GetSize());
      CryptoPP::AutoSeededRandomPool prng;
      prng.GenerateBlock(nonce, nonce.size());
      uint8_t data[GetSize()];
      CryptoPP::ArraySource
        source(nonce, nonce.size(), true, new CryptoPP::HexEncoder(new CryptoPP::ArraySink(data, GetSize())));
      return Hash(data);
    }
  };
}

#endif //TOKEN_HASH_H