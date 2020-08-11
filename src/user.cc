#include "user.h"

namespace Token{
    UserID::UserID(const std::string& value):
        data_(){
        strncpy(data_, value.data(), kMaxLength);
    }

    UserID::UserID(ByteBuffer* bytes):
        data_(){
        if(!bytes->GetBytes((uint8_t*)data_, kMaxLength))
            LOG(WARNING) << "failed to parse user id from byte array";
    }

    std::string UserID::Get() const{
        return std::string(data_, kMaxLength);
    }

    bool UserID::Encode(ByteBuffer* bytes) const{
        bytes->PutBytes((uint8_t*)data_, kMaxLength);
        return true;
    }
}