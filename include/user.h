#ifndef TOKEN_USER_H
#define TOKEN_USER_H

#include <string>
#include <sstream>
#include <cstring>
#include <leveldb/slice.h>

#include "json.h"
#include "binary_type.h"

namespace token{
  static const uint64_t kUserSize = 64;
  class User : public BinaryType<kUserSize>{
   public:
    User(): BinaryType<kUserSize>(){}
    User(const uint8_t* data, const uint64_t& size):
      BinaryType<kUserSize>(data, size){}
    explicit User(const char* value):
      BinaryType<kUserSize>((const uint8_t*)value, std::min((uint64_t)strlen(value), kUserSize)){}
    explicit User(const std::string& value):
      BinaryType<kUserSize>((const uint8_t*)value.data(), std::min((uint64_t)value.length(), kUserSize)){}
    explicit User(const leveldb::Slice& value):
      BinaryType<kUserSize>((const uint8_t*)value.data(), std::min((uint64_t)value.size(), kUserSize)){}
    User(const User& user) = default;
    ~User() override = default;

    std::string ToString() const override{
      std::stringstream ss;
      ss << "User(" << data() << ")";
      return ss.str();
    }

    User& operator=(const User& other) = default;

    friend bool operator==(const User& a, const User& b){
      return Compare(a, b) == 0;
    }

    friend bool operator!=(const User& a, const User& b){
      return Compare(a, b) != 0;
    }

    friend bool operator<(const User& a, const User& b){
      return Compare(a, b) < 0;
    }

    friend bool operator>(const User& a, const User& b){
      return Compare(a, b) > 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const User& user){
      return stream << user.ToString();
    }

    operator leveldb::Slice() const{
      return leveldb::Slice(data(), size());
    }

    static inline int
    Compare(const User& a, const User& b){
      return BinaryType<kUserSize>::Compare(a, b);
    }
  };

  namespace json{
    static inline bool
    Write(Writer& writer, const User& val){
      JSON_STRING(writer, val.ToString());
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const User& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}

#endif//TOKEN_USER_H