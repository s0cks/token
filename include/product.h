#ifndef TOKEN_PRODUCT_H
#define TOKEN_PRODUCT_H

#include "object.h"

namespace token{
  static const int64_t kBytesForProduct = 64;
  class Product : public RawType<kBytesForProduct>{
    using Base = RawType<kBytesForProduct>;
   public:
    Product():
      Base(){}
    Product(const uint8_t* bytes, int64_t size):
      Base(bytes, size){}
    Product(const char* value):
      Base(){
      memcpy(data(), value, std::min((int64_t)strlen(value), kBytesForProduct));
    }
    Product(const std::string& value):
      Base(){
      memcpy(data(), value.data(), std::min((int64_t) value.length(), Base::GetSize()));
    }
    Product(const Product& product):
      Base(){
      memcpy(data(), product.data(), Base::GetSize());
    }
    ~Product() = default;

    void operator=(const Product& product){
      memcpy(data(), product.data(), Base::GetSize());
    }

    friend bool operator==(const Product& a, const Product& b){
      return Base::Compare(a, b) == 0;
    }

    friend bool operator!=(const Product& a, const Product& b){
      return Base::Compare(a, b) != 0;
    }

    friend int operator<(const Product& a, const Product& b){
      return Base::Compare(a, b);
    }

    friend std::ostream& operator<<(std::ostream& stream, const Product& product){
      stream << product.str();
      return stream;
    }
  };
}

#endif//TOKEN_PRODUCT_H