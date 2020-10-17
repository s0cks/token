#include "product.h"

namespace Token{
    Product::Product(const std::string& value):
        data_(){
        strncpy(data_, value.data(), value.length());
        if(value.length() < kSize)
            memset(&data_[value.length()], 0, kSize-value.length());
    }

    Product::Product(ByteBuffer* bytes):
            data_(){
        if(!bytes->GetBytes((uint8_t*)data_, kSize))
            LOG(WARNING) << "failed to parse id from byte array";
    }

    std::string Product::Get() const{
        return std::string(data_, kSize);
    }

    bool Product::Encode(ByteBuffer* bytes) const{
        bytes->PutBytes((uint8_t*)data_, kSize);
        return true;
    }
}