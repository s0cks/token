#ifndef TOKEN_PRODUCT_H
#define TOKEN_PRODUCT_H

#include "common.h"
#include "byte_buffer.h"

namespace Token{
    class Product{
    public:
        static const size_t kSize = 64; // alias
    private:
        char data_[64];
    public:
        Product() = default;
        Product(const Product& Product):
            data_(){
            memcpy(data_, Product.data_, kSize);
        }
        Product(const std::string& value);
        Product(ByteBuffer* bytes);
        ~Product() = default;

        std::string Get() const;
        bool Encode(ByteBuffer* bytes) const;

        void operator=(const Product& Product){
            memcpy(data_, Product.data_, 64);
        }

        void operator=(const std::string& Product){
            memcpy(data_, Product.data(), 64);
        }

        friend bool operator==(const Product& a, const Product& b){
            return strncmp(a.data_, b.data_, 64) == 0;
        }

        friend bool operator==(const Product& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64)) == 0;
        }

        friend bool operator!=(const Product& a, const Product& b){
            return !operator==(a, b);
        }

        friend bool operator!=(const Product& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64)) != 0;
        }

        friend int operator<(const Product& a, const Product& b){
            return strncmp(a.data_, b.data_, 64);
        }

        friend int operator<(const Product& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64));
        }

        friend std::ostream& operator<<(std::ostream& stream, const Product& Product){
            stream << std::string(Product.data_, 64);
            return stream;
        }
    };
}

#endif //TOKEN_PRODUCT_H
