#ifndef TOKEN_PRODUCT_H
#define TOKEN_PRODUCT_H

#include <string>
#include <cstring>
#include <sstream>

#include "binary_type.h"

namespace token{
  static const uint64_t kProductSize = 64;
  class Product : public BinaryType<kProductSize>{
   public:
    Product(): BinaryType<kProductSize>(){}
    Product(const uint8_t* data, const uint64_t& size): BinaryType<kProductSize>(data, size){}
    explicit Product(const char* data): BinaryType<kProductSize>((const uint8_t*)data, std::min((uint64_t)strlen(data), kProductSize)){}
    explicit Product(const std::string& data): BinaryType<kProductSize>((const uint8_t*)data.data(), std::min((uint64_t)data.length(), kProductSize)){}
    Product(const Product& other) = default;
    ~Product() override = default;

    std::string ToString() const override{
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

    static inline int Compare(const Product& a, const Product& b){
      return BinaryType<kProductSize>::Compare(a, b);
    }
  };
}

#endif//TOKEN_PRODUCT_H