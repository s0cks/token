#include "product.h"

namespace Token{
    Product::Product(const std::string& value):
        data_(){
        strncpy(data_, value.data(), value.length());
        if(value.length() < kSize)
            memset(&data_[value.length()], 0, kSize-value.length()-1);
    }

    std::string Product::Get() const{
        return std::string(data_, strlen(data_));
    }
}