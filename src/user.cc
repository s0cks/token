#include "user.h"

namespace Token{
    User::User(const std::string& value):
        data_(){
        memcpy(data_, value.data(), value.length());
        if(value.length() < kSize){
            memset(&data_[value.length()], 0, kSize-value.length()-1);
        }
    }

    std::string User::Get() const{
        return std::string(data(), kSize);
    }
}