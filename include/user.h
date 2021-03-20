#ifndef TOKEN_USER_H
#define TOKEN_USER_H

#include "object.h"

namespace token{
  static const int64_t kRawUserSize = 64;
  class User : public RawType<kRawUserSize>{
    using Base = RawType<kRawUserSize>;
   public:
    User():
      Base(){}
    User(const uint8_t* bytes, int64_t size):
      Base(bytes, size){}
    explicit User(const char* value):
      Base(){
      memcpy(data(), value, std::min((int64_t)strlen(value), Base::GetSize()));
    }
    explicit User(const std::string& value):
      Base(){
      memcpy(data(), value.data(), std::min((int64_t) value.length(), Base::GetSize()));
    }
    User(const User& user):
      Base(user){
      memcpy(data(), user.data(), Base::GetSize());
    }
    ~User() override = default;

    User& operator=(const User& user){
      memcpy(data(), user.data(), Base::GetSize());
      return (*this);
    }

    friend bool operator==(const User& a, const User& b){
      return Compare(a, b) == 0;
    }

    friend bool operator!=(const User& a, const User& b){
      return Compare(a, b) != 0;
    }

    friend bool operator<(const User& a, const User& b){
      return Compare(a, b) < 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const User& user){
      stream << user.str();
      return stream;
    }

    static int Compare(const User& a, const User& b){
      return Base::Compare(a, b);
    }
  };
}

#endif//TOKEN_USER_H