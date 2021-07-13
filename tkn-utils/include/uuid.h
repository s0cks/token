#ifndef TOKEN_UUID_H
#define TOKEN_UUID_H

#include <cassert>
#include <uuid/uuid.h>
#include <leveldb/slice.h>

#include "json.h"

namespace token{
  namespace internal{
    static const size_t kMaxUUIDLength = 16;
    static const size_t kMaxUUIDStringLength = 37;
  }
  class UUID{
   private:
    uint8_t data_[internal::kMaxUUIDLength];

    const inline uint8_t*
    raw() const{
      return data_;
    }
   public:
    UUID():
      data_(){
      memset(data_, 0, internal::kMaxUUIDLength);
    }
    UUID(const uint8_t* data, const size_t& size):
      UUID(){
      memcpy(data_, data, std::min(size, internal::kMaxUUIDLength));
    }
    explicit UUID(const std::string& data):
      UUID((uint8_t*)data.data(), data.length()){}
    explicit UUID(const leveldb::Slice& data):
      UUID((uint8_t*)data.data(), data.size()){}
    UUID(const UUID& other) = default;
    ~UUID() = default;

    const char* data() const{
      return (const char*)raw();
    }

    int64_t size() const{
      return internal::kMaxUUIDLength;
    }

    std::string ToString() const{
      char uuid_str[internal::kMaxUUIDStringLength];
      uuid_unparse(data_, uuid_str);
      return std::string(uuid_str, internal::kMaxUUIDStringLength);
    }

    UUID& operator=(const UUID& other){
      if(this == &other)
        return (*this);
      memset(data_, 0, sizeof(uint8_t)*internal::kMaxUUIDLength);
      memcpy(data_, other.raw(), sizeof(uint8_t)*internal::kMaxUUIDLength);
      return (*this);
    }

    friend bool operator==(const UUID& a, const UUID& b){
      return uuid_compare(a.raw(), b.raw()) == 0;
    }

    friend bool operator!=(const UUID& a, const UUID& b){
      return uuid_compare(a.data_, b.data_) != 0;
    }

    friend bool operator<(const UUID& a, const UUID& b){
      return uuid_compare(a.data_, b.data_) < 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const UUID& uuid){
      return stream << uuid.ToString();
    }

    explicit operator leveldb::Slice() const{
      return leveldb::Slice((char*)data_, size());
    }
  };

  namespace json{
    static inline bool
    Write(Writer& writer, const UUID& val){
      JSON_STRING(writer, val.ToString());
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const UUID& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}

#endif//TOKEN_UUID_H