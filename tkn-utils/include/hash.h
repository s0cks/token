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

#include "json.h"
#include "crypto.h"

namespace token{
  class Hash{
    //TODO:
    // - refactor for RawType class
   public:
    static const int64_t kSize = 256 / 8;
   public:
    static inline int
    Compare(const Hash& a, const Hash& b){
      return memcmp(a.data(), b.data(), kSize);
    }

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

    struct Comparator{
      bool operator()(const Hash& a, const Hash& b) const{
        return a < b;
      }
    };

    struct Equal{
      bool operator()(const Hash& a, const Hash& b) const{
        return a == b;
      }
    };
   private:
    uint8_t data_[kSize];

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
    explicit Hash(const unsigned char* data):
        data_(){
      memcpy(data_, data, sizeof(data_));
    }
    explicit Hash(const std::string& slice):
      data_(){
      memcpy(data(), slice.data(), std::min(static_cast<int64_t>(slice.size()), GetSize()));
    }

    explicit Hash(uint8_t* data, int64_t size = Hash::GetSize()):
      data_(){
      memcpy(data_, data, size);
    }
    explicit Hash(const uint8_t* data, int64_t size = Hash::GetSize()):
      data_(){
      memcpy(data_, data, size);
    }
    explicit Hash(const leveldb::Slice& slice, int64_t size = Hash::GetSize()):
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

    uint8_t* raw_data() const{
      return (uint8_t*)data_;
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

    friend inline bool operator==(const Hash& a, const Hash& b){
      return Compare(a, b) == 0;
    }

    friend inline bool operator!=(const Hash& a, const Hash& b){
      return Compare(a, b) != 0;
    }

    friend inline bool operator<(const Hash& a, const Hash& b){
      return Compare(a, b) < 0;
    }

    friend inline bool operator>(const Hash& a, const Hash& b){
      return Compare(a, b) > 0;
    }

    Hash& operator=(const Hash& rhs){
      if(&rhs == this)
        return (*this);
      memcpy(data_, rhs.data_, kSize);
      return (*this);
    }

    explicit operator leveldb::Slice() const{
      return leveldb::Slice((const char*)data_, kSize);
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
      int64_t size = GetSize();
      uint8_t data[size];
      CryptoPP::StringSource source(hex, true, new CryptoPP::HexDecoder(new CryptoPP::ArraySink(data, size)));
      return Hash(data, size);
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

    static inline Hash
    FromSlice(const std::string& slice){
      return Hash(slice.data(), std::min((int64_t)slice.size(), GetSize()));
    }

    template<typename HashFunction>
    static inline Hash
    ComputeHash(const uint8_t* data, const size_t& size){
      HashFunction function;
      CryptoPP::SecByteBlock digest(HashFunction::DIGESTSIZE);
      CryptoPP::ArraySource source(data, size, true, new CryptoPP::HashFilter(function, new CryptoPP::ArraySink(digest.data(), digest.size())));
      return Hash(digest.data(), HashFunction::DIGESTSIZE);
    }
  };

  typedef std::vector<Hash> HashList;
  typedef std::set<Hash, Hash::Comparator> HashSet;

  template<class T>
  static inline HashList&
  operator<<(HashList& list, const std::shared_ptr<T>& val){
    list.push_back(val->GetHash());
    return list;
  }

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
}

#endif //TOKEN_HASH_H