#ifndef TOKEN_VERSION_H
#define TOKEN_VERSION_H

#include <cstring>
#include <sstream>
#include "binary_type.h"

namespace token{
#define TOKEN_VERSION_MAJOR 0
#define TOKEN_VERSION_MINOR 0
#define TOKEN_VERSION_REVISION 1

  static const int64_t kVersionSize = sizeof(int16_t) + sizeof(int16_t) + sizeof(int16_t);
  class Version : public BinaryType<kVersionSize>{
    //TODO: create Encoder & Decoder
   public:
    static inline int
    Compare(const Version& a, const Version& b){
      if(a.major() < b.major()){
        return -1;
      } else if(a.major() > b.major()){
        return +1;
      }

      if(a.minor() < b.minor()){
        return -1;
      } else if(a.minor() > b.minor()){
        return +1;
      }

      if(a.revision() < b.revision()){
        return -1;
      } else if(a.revision() > b.revision()){
        return +1;
      }
      return 0;
    }
   private:
    enum Layout{
      kMajorPosition = 0,
      kBytesForMajor = sizeof(int16_t),

      kMinorPosition = kMajorPosition+kBytesForMajor,
      kBytesForMinor = sizeof(int16_t),

      kRevisionPosition = kMinorPosition+kBytesForMinor,
      kBytesForRevision = sizeof(int16_t),
    };

    inline void
    SetMajor(const int16_t& val){
      memcpy(&data_[kMajorPosition], &val, kBytesForMajor);
    }

    inline void
    SetMinor(const int16_t& val){
      memcpy(&data_[kMinorPosition], &val, kBytesForMinor);
    }

    inline void
    SetRevision(const int16_t& val){
      memcpy(&data_[kRevisionPosition], &val, kBytesForRevision);
    }
   public:
    Version() = default;
    Version(const int16_t& major, const int16_t& minor, const int16_t& revision):
      BinaryType<kVersionSize>(){
      SetMajor(major);
      SetMinor(minor);
      SetRevision(revision);
    }
    Version(const uint8_t* data, const int64_t& size):
      BinaryType<kVersionSize>(data, size){}
    Version(const Version& version) = default;
    ~Version() override = default;

    int16_t major() const{
      return *((int16_t*)&data_[kMajorPosition]);
    }

    int16_t minor() const{
      return *((int16_t*)&data_[kMinorPosition]);
    }

    int16_t revision() const{
      return *((int16_t*)&data_[kRevisionPosition]);
    }

    std::string ToString() const override{
      std::stringstream ss;
      ss << major() << "." << minor() << "." << revision();
      return ss.str();
    }

    Version& operator=(const Version& other) = default;

    friend bool operator==(const Version& a, const Version& b){
      return Compare(a, b) == 0;
    }

    friend bool operator!=(const Version& a, const Version& b){
      return Compare(a, b) != 0;
    }

    friend bool operator<=(const Version& a, const Version& b){
      return Compare(a, b) <= 0;
    }

    friend bool operator<(const Version& a, const Version& b){
      return Compare(a, b) < 0;
    }

    friend bool operator>=(const Version& a, const Version& b){
      return Compare(a, b) >= 0;
    }

    friend bool operator>(const Version& a, const Version& b){
      return Compare(a, b) > 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Version& version){
      return stream << version.ToString();
    }

    static inline Version
    CurrentVersion(){
      return Version(TOKEN_VERSION_MAJOR, TOKEN_VERSION_MINOR, TOKEN_VERSION_REVISION);
    }

    static inline int64_t
    GetSize(){
      return kVersionSize;
    }
  };
}

#endif //TOKEN_VERSION_H