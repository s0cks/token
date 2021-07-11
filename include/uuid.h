#ifndef TOKEN_UUID_H
#define TOKEN_UUID_H

#include <cassert>
#include <uuid/uuid.h>
#include <leveldb/slice.h>

#include "json.h"
#include "codec.h"
#include "encoder.h"
#include "decoder.h"

namespace token{
  namespace internal{
    static const size_t kMaxUUIDLength = 16;
    static const size_t kMaxUUIDStringLength = 37;
  }
  class UUID : public Object{
   public:
   class Encoder : public codec::EncoderBase<UUID>{
    public:
     Encoder(const UUID& value, const codec::EncoderFlags& flags):
      codec::EncoderBase<UUID>(value, flags){}
      Encoder(const Encoder& other) = default;
     ~Encoder() override = default;

     int64_t GetBufferSize() const override{
       int64_t size = BaseType::GetBufferSize();
       size += internal::kMaxUUIDLength;
       return size;
     }

     bool Encode(const BufferPtr& buff) const override{
       if(!BaseType::Encode(buff))
         return false;
       const auto& raw = value().raw();
       if(!buff->PutBytes(raw, internal::kMaxUUIDLength)){
         LOG(FATAL) << "cannot encode raw UUID to buffer.";
         return false;
       }
       return true;
     }

     Encoder& operator=(const Encoder& other) = default;
   };
   class Decoder : public codec::DecoderBase<UUID>{
    public:
     explicit Decoder(const codec::DecoderHints& hints):
      codec::DecoderBase<UUID>(hints){}
      Decoder(const Decoder& other) = default;
     ~Decoder() override = default;

     bool Decode(const BufferPtr& buff, UUID& result) const override{
       uint8_t raw[internal::kMaxUUIDLength];
       if(!buff->GetBytes(raw, internal::kMaxUUIDLength)){
         LOG(FATAL) << "cannot decode raw uuid from buffer.";
         return false;
       }

       result = UUID(raw, internal::kMaxUUIDLength);
       return true;
     }

     Decoder& operator=(const Decoder& other) = default;
   };
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
    ~UUID() override = default;

    Type type() const override{
      return Type::kUUID;
    }

    const char* data() const{
      return (const char*)raw();
    }

    int64_t size() const{
      return internal::kMaxUUIDLength;
    }

    std::string ToString() const override{
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