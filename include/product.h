#ifndef TOKEN_PRODUCT_H
#define TOKEN_PRODUCT_H

#include <string>
#include <cstring>
#include <sstream>

#include "json.h"
#include "codec.h"
#include "decoder.h"
#include "encoder.h"

namespace token{
  static const int64_t kMaxProductLength = 64;
  class Product : public Object{
   private:
    static inline int Compare(const Product& a, const Product& b){
      return strncmp(a.data(), b.data(), kMaxProductLength);
    }
   public:
   class Encoder : public codec::EncoderBase<Product>{
     public:
      Encoder(const Product& value, const codec::EncoderFlags& flags):
        codec::EncoderBase<Product>(value, flags){}
      Encoder(const Encoder& other) = default;
      ~Encoder() override = default;

      int64_t GetBufferSize() const override{
        int64_t size = BaseType::GetBufferSize();
        size += kMaxProductLength;
        return size;
      }

      bool Encode(const BufferPtr& buff) const override{
        if(!BaseType::Encode(buff))
          return false;
        const auto& data = value().data();
        return buff->PutBytes((uint8_t*)data, kMaxProductLength);
      }

      Encoder& operator=(const Encoder& other) = default;
    };
   class Decoder : public codec::DecoderBase<Product>{
    public:
     explicit Decoder(const codec::DecoderHints& hints):
      codec::DecoderBase<Product>(hints){}
      Decoder(const Decoder& other) = default;
     ~Decoder() override = default;

     bool Decode(const BufferPtr& buff, Product& result) const override{
       //TODO: decode type
       //TODO: decode version

       uint8_t data[kMaxProductLength];
       if(!buff->GetBytes(data, kMaxProductLength)){
         LOG(FATAL) << "couldn't decode product bytes from buffer.";
         return false;
       }

       result = Product(data, kMaxProductLength);
       return true;
     }

     Decoder& operator=(const Decoder& other) = default;
   };
   private:
    char data_[kMaxProductLength];
   public:
    Product():
      data_(){
      memset(data_, 0, kMaxProductLength);
    }
    Product(uint8_t* data, const int64_t& size):
      Product(){
      memcpy(data_, data, std::min(size, kMaxProductLength));
    }
    explicit Product(const std::string& data):
      Product((uint8_t*)data.data(), static_cast<int64_t>(data.length())){}
    ~Product() override = default;

    Type type() const override{
      return Type::kProduct;
    }

    const char* data() const{
      return data_;
    }

    size_t size() const{
      return kMaxProductLength;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << "Product(" << data() << ")";
      return ss.str();
    }

    Product& operator=(const Product& other) = default;

    friend bool operator==(const Product& a, const Product& b){
      return Compare(a, b) == 0;
    }

    friend bool operator!=(const Product& a, const Product& b){
      return Compare(a, b) != 0;
    }

    friend bool operator<(const Product& a, const Product& b){
      return Compare(a, b) < 0;
    }

    friend bool operator>(const Product& a, const Product& b){
      return Compare(a, b) > 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Product& product){
      return stream << product.ToString();
    }
  };

  namespace json{
    static inline bool
    Write(Writer& writer, const Product& val){
      JSON_STRING(writer, val.ToString());
      return true;
    }

    static inline bool
    SetField(Writer& writer, const char* name, const Product& val){
      JSON_KEY(writer, name);
      return Write(writer, val);
    }
  }
}

#endif//TOKEN_PRODUCT_H