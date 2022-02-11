#ifndef TOKEN_USER_H
#define TOKEN_USER_H

#include <string>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <leveldb/slice.h>

namespace token{
  class User{
    /**
     * Fixed-length string representing a User Id.
     */
   public:
    static constexpr const uint64_t kMaxLength = 64;
    static constexpr const uint64_t kSize = kMaxLength;

    static inline int
    Compare(const User& lhs, const User& rhs){
      return strncmp(lhs.c_str(), rhs.c_str(), kSize);
    }

    static inline int
    Compare(const User& lhs, const std::string& rhs){
      return strncmp(lhs.c_str(), rhs.c_str(), kSize);
    }
   private:
    uint8_t data_[kSize];

    inline void
    CopyFrom(const uint8_t* data, uint64_t size){
      memcpy(data_, data, std::min(size, kSize));
    }
   public:
    User():
      data_(){
      clear();
    }
    User(const uint8_t* data, uint64_t length):
      data_(){
      CopyFrom(data, length);
    }
    User(const char* data, uint64_t length):
      User((const uint8_t*)data, length){
    }
    explicit User(const std::string& val):
      User(val.data(), val.length()){
    }
    User(const User& rhs):
      data_(){
      CopyFrom(rhs.data(), rhs.length());
    }
    ~User() = default;

    const uint8_t* data() const{
      return data_;
    }

    const char* c_str() const{
      return (const char*)data();
    }

    uint64_t length() const{
      return strlen(c_str());
    }

    void clear() const{
      memset((void*)data_, 0, kSize);
    }

    const uint8_t* begin() const{
      return data_;
    }

    const uint8_t* end() const{
      return data() + kSize;
    }

    bool empty() const{
      return std::all_of(begin(), end(), [](const uint8_t val){
        return val == 0;
      });
    }

    User& operator=(const std::string& val){
      //TODO: check for redundant assignments
      CopyFrom((const uint8_t*)val.data(), val.length());
      return *this;
    }

    explicit operator std::string() const{
      return { c_str(), length() };
    }

    explicit operator leveldb::Slice() const{
      return { c_str(), length() };
    }

    friend std::ostream& operator<<(std::ostream& stream, const User& val){
      return stream << "User(" << std::string((const char*)val.data(), val.length()) << ")";
    }

    friend bool operator==(const User& lhs, const User& rhs){
      return Compare(lhs, rhs) == 0;
    }

    friend bool operator==(const User& lhs, const std::string& rhs){
      return Compare(lhs, rhs) == 0;
    }

    friend bool operator!=(const User& lhs, const User& rhs){
      return Compare(lhs, rhs) != 0;
    }

    friend bool operator!=(const User& lhs, const std::string& rhs){
      return Compare(lhs, rhs) != 0;
    }

    friend bool operator<(const User& lhs, const User& rhs){
      return Compare(lhs, rhs) < 0;
    }

    friend bool operator<(const User& lhs, const std::string& rhs){
      return Compare(lhs, rhs) < 0;
    }

    friend bool operator>(const User& lhs, const User& rhs){
      return Compare(lhs, rhs) > 0;
    }

    friend bool operator>(const User& lhs, const std::string& rhs){
      return Compare(lhs, rhs) < 0;
    }
  };
}

#endif//TOKEN_USER_H