#ifndef TOKEN_UUID_H
#define TOKEN_UUID_H

#include <uuid/uuid.h>
#include "object.h"

namespace token{
  static const int16_t kRawUUIDSize = 16;
  class UUID : public RawType<kRawUUIDSize>{
   public:
    UUID():
      RawType(){
      uuid_generate_time_safe(data_);
    }
    UUID(uint8_t* bytes, int64_t size):
      RawType(bytes, size){}
    explicit UUID(const char* uuid):
      RawType(){
      uuid_parse(uuid, data_);
    }
    explicit UUID(const std::string& uuid):
      RawType(){
      memcpy(data(), uuid.data(), kRawUUIDSize);
    }
    explicit UUID(const leveldb::Slice& slice):
      RawType(){
      memcpy(data(), slice.data(), kRawUUIDSize);
    }
    UUID(const UUID& other):
      RawType(other){
      memcpy(data(), other.data(), kRawUUIDSize);
    }
    ~UUID() override = default;

    std::string ToStringAbbreviated() const{
      char uuid_str[37];
      uuid_unparse(data_, uuid_str);
      return std::string(uuid_str, 8);
    }

    std::string ToString() const{
      char uuid_str[37];
      uuid_unparse(data_, uuid_str);
      return std::string(uuid_str, 37);
    }

    UUID& operator=(const UUID& other){
      memcpy(data(), other.data(), kRawUUIDSize);
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

    explicit operator leveldb::Slice() const{
      return leveldb::Slice(data(), size());
    }
  };
}

#endif//TOKEN_UUID_H