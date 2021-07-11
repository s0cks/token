#ifndef TOKEN_USER_H
#define TOKEN_USER_H

#include <string>
#include <sstream>
#include <cstring>
#include <leveldb/slice.h>

#include "json.h"
#include "codec.h"
#include "encoder.h"
#include "decoder.h"

namespace token{
  namespace internal{
    static const size_t kMaxUserLength = 64;
  }

  class User : public Object{
   public:
    static inline int
    Compare(const User& a, const User& b){
      return strncmp(a.data(), b.data(), internal::kMaxUserLength);
    }

   class Encoder : public codec::EncoderBase<User>{
     public:
      Encoder(const User& value, const codec::EncoderFlags& flags):
        codec::EncoderBase<User>(value, flags){}
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;

      int64_t GetBufferSize() const override{
        int64_t size = BaseType::GetBufferSize();
        size += internal::kMaxUserLength;
        return size;
      }

      bool Encode(const BufferPtr& buff) const override{
        if(!BaseType::Encode(buff))
          return false;
        const auto& data = value().data();
        return buff->PutBytes((uint8_t*)data, static_cast<int64_t>(internal::kMaxUserLength));
      }

      Encoder& operator=(const Encoder& other) = default;
  };

   class Decoder : public codec::DecoderBase<User>{
    public:
     explicit Decoder(const codec::DecoderHints& hints):
      codec::DecoderBase<User>(hints){}
      Decoder(const Decoder& other) = default;
     ~Decoder() override = default;

     bool Decode(const BufferPtr& buff, User& result) const override{
       //TODO: decode type
       //TODO: decode version
       uint8_t bytes[internal::kMaxUserLength];
       if(!buff->GetBytes(bytes, internal::kMaxUserLength)){
         LOG(FATAL) << "couldn't decode user bytes from buffer.";
         return false;
       }

       result = User(bytes, internal::kMaxUserLength);
       return true;
     }

     Decoder& operator=(const Decoder& other) = default;
   };
   private:
    char data_[internal::kMaxUserLength];
   public:
    User():
      data_(){
      memset(data_, 0, internal::kMaxUserLength);
    }
    User(uint8_t* data, const size_t& size):
      User(){
      memcpy(data_, data, std::min(internal::kMaxUserLength, size));
    }
    explicit User(const char* data):
      User((uint8_t*)data, strlen(data)){}
    explicit User(const std::string& data):
      User((uint8_t*)data.data(), static_cast<int64_t>(data.length())){}
    explicit User(const leveldb::Slice& data):
      User((uint8_t*)data.data(), static_cast<int64_t>(data.size())){}
    ~User() override = default;

    Type type() const override{
      return Type::kUser;
    }

    const char* data() const{
      return data_;
    }

    size_t size() const{
      return internal::kMaxUserLength;
    }

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

    explicit operator leveldb::Slice() const{
      return leveldb::Slice(data(), size());
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