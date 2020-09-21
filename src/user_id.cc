#include "user_id.h"

namespace Token{
    UserID::UserID(const std::string& value):
        data_(){
        strncpy(data_, value.data(), value.length());
        if(value.length() < 64){
            memset(&data_[value.length()], 0, 64-value.length());
        }
    }

    UserID::UserID(ByteBuffer* bytes):
        data_(){
        if(!bytes->GetBytes((uint8_t*)data_, 64))
            LOG(WARNING) << "failed to parse user id from byte array";
    }

    std::string UserID::Get() const{
        return std::string(data_, 64);
    }

    bool UserID::Encode(ByteBuffer* bytes) const{
        bytes->PutBytes((uint8_t*)data_, 64);
        return true;
    }
}