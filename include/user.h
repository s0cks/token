#ifndef TOKEN_USER_H
#define TOKEN_USER_H

#include "common.h"

namespace Token{
    class User{
    public:
        static const intptr_t kSize = 64; // alias
    private:
        uint8_t data_[kSize];
    public:
        User() = default;
        User(const uint8_t* bytes):
            data_(){
            memcpy(data_, bytes, kSize);
        }
        User(const User& user):
            data_(){
            memcpy(data_, user.data_, kSize);
        }
        User(const std::string& value);
        ~User() = default;

        const char* data() const{
            return (char*)data_;
        }

        std::string Get() const;

        void operator=(const User& user){
            memcpy(data_, user.data_, 64);
        }

        void operator=(const std::string& user){
            memcpy(data_, user.data(), 64);
        }

        friend bool operator==(const User& a, const User& b){
            return memcmp(a.data_, b.data_, kSize) == 0;
        }

        friend bool operator==(const User& a, const std::string& b){
            return memcmp(a.data_, b.data(), std::min(b.length(), (unsigned long)kSize)) == 0;
        }

        friend bool operator!=(const User& a, const User& b){
            return !operator==(a, b);
        }

        friend bool operator!=(const User& a, const std::string& b){
            return memcmp(a.data_, b.data(), std::min(b.length(), (unsigned long)kSize)) != 0;
        }

        friend int operator<(const User& a, const User& b){
            return memcmp(a.data_, b.data(), kSize);
        }

        friend int operator<(const User& a, const std::string& b){
            return memcmp(a.data_, b.data(), std::min(b.length(), (unsigned long)kSize));
        }

        friend std::ostream& operator<<(std::ostream& stream, const User& user){
            stream << std::string((char*)user.data_, kSize);
            return stream;
        }
    };
}

#endif //TOKEN_USER_H
