#ifndef TOKEN_UUID_H
#define TOKEN_UUID_H

#include <cassert>
#include <uuid/uuid.h>
#include "binary_type.h"

namespace token{
  static const int64_t kUUIDSize = 16;
  class UUID : public BinaryType<kUUIDSize>{
   public:
    UUID():
      BinaryType<kUUIDSize>(){
      uuid_generate_time_safe(data_);
    }
    UUID(const uint8_t* data, const int64_t& size):
      BinaryType<kUUIDSize>(){
      memset(data_, 0, GetSize());
      assert(size == GetSize());
      memcpy(data_, data, GetSize());
    }
    UUID(const char* data):
      BinaryType<kUUIDSize>(){
      uuid_parse(data, data_);
    }
    UUID(const std::string& data):
      BinaryType<kUUIDSize>(){
      memset(data_, 0, GetSize());
      assert(((int64_t)data.length()) == GetSize());
      memcpy(data_, data.data(), GetSize());
    }
    UUID(const UUID& other):
      BinaryType<kUUIDSize>(){
      memset(data_, 0, GetSize());
      memcpy(data_, other.data_, GetSize());
    }
    ~UUID() override = default;

    std::string ToStringAbbreviated() const{
      char uuid_str[37];
      uuid_unparse(data_, uuid_str);
      return std::string(uuid_str, 8);
    }

    std::string ToString() const override{
      char uuid_str[37];
      uuid_unparse(data_, uuid_str);
      return std::string(uuid_str, 37);
    }

    UUID& operator=(const UUID& other){
      memset(data_, 0, GetSize());
      memcpy(data_, other.data_, GetSize());
      return (*this);
    }

    friend bool operator==(const UUID& a, const UUID& b){
      return uuid_compare(a.data_, b.data_) == 0;
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
  };
}

#endif//TOKEN_UUID_H