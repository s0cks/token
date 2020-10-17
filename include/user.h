#ifndef TOKEN_USER_H
#define TOKEN_USER_H

#include "common.h"
#include "byte_buffer.h"

namespace Token{
    class User{
    public:
        static const size_t kSize = 64; // alias
    private:
        char data_[64];
    public:
        User() = default;
        User(const User& user):
            data_(){
            memcpy(data_, user.data_, kSize);
        }
        User(const std::string& value);
        User(ByteBuffer* bytes);
        ~User() = default;

        std::string Get() const;
        bool Encode(ByteBuffer* bytes) const;

        void operator=(const User& user){
            memcpy(data_, user.data_, 64);
        }

        void operator=(const std::string& user){
            memcpy(data_, user.data(), 64);
        }

        friend bool operator==(const User& a, const User& b){
            return strncmp(a.data_, b.data_, 64) == 0;
        }

        friend bool operator==(const User& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64)) == 0;
        }

        friend bool operator!=(const User& a, const User& b){
            return !operator==(a, b);
        }

        friend bool operator!=(const User& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64)) != 0;
        }

        friend int operator<(const User& a, const User& b){
            return strncmp(a.data_, b.data_, 64);
        }

        friend int operator<(const User& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64));
        }

        friend std::ostream& operator<<(std::ostream& stream, const User& user){
            stream << std::string(user.data_, 64);
            return stream;
        }
    };
}

#endif //TOKEN_USER_H
