#include "token_id.h"

namespace Token{
    TokenID::TokenID(const std::string& value):
        data_(){
        strncpy(data_, value.data(), value.length());
        if(value.length() < kSize)
            memset(&data_[value.length()], 0, kSize-value.length());
    }

    TokenID::TokenID(ByteBuffer* bytes):
            data_(){
        if(!bytes->GetBytes((uint8_t*)data_, kSize))
            LOG(WARNING) << "failed to parse id from byte array";
    }

    std::string TokenID::Get() const{
        return std::string(data_, kSize);
    }

    bool TokenID::Encode(ByteBuffer* bytes) const{
        bytes->PutBytes((uint8_t*)data_, kSize);
        return true;
    }
}