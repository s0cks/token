#include "user.h"

namespace Token{
    User::User(const std::string& value):
        data_(){
        strncpy(data_, value.data(), value.length());
        if(value.length() < 64){
            memset(&data_[value.length()], 0, 64-value.length());
        }
    }

    User::User(ByteBuffer* bytes):
        data_(){
        if(!bytes->GetBytes((uint8_t*)data_, 64))
            LOG(WARNING) << "failed to parse user id from byte array";
    }

    std::string User::Get() const{
        return std::string(data_, 64);
    }

    bool User::Encode(ByteBuffer* bytes) const{
        bytes->PutBytes((uint8_t*)data_, 64);
        return true;
    }
}