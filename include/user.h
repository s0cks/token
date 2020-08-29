#ifndef TOKEN_USER_H
#define TOKEN_USER_H

#include "common.h"
#include "bytes.h"

namespace Token{
    class UserID{
    public:
        static const size_t kSize = 64; // alias
    private:
        char data_[64];
    public:
        UserID() = default;
        UserID(const std::string& value);
        UserID(ByteBuffer* bytes);
        ~UserID() = default;

        std::string Get() const;
        bool Encode(ByteBuffer* bytes) const;

        void operator=(const UserID& user){
            memcpy(data_, user.data_, 64);
        }

        void operator=(const std::string& user){
            memcpy(data_, user.data(), 64);
        }

        friend bool operator==(const UserID& a, const UserID& b){
            return strncmp(a.data_, b.data_, 64) == 0;
        }

        friend bool operator==(const UserID& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64)) == 0;
        }

        friend bool operator!=(const UserID& a, const UserID& b){
            return !operator==(a, b);
        }

        friend bool operator!=(const UserID& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64)) == 0;
        }

        friend int operator<(const UserID& a, const UserID& b){
            return strncmp(a.data_, b.data_, 64);
        }

        friend int operator<(const UserID& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64));
        }

        friend std::ostream& operator<<(std::ostream& stream, const UserID& user){
            stream << std::string(user.data_, 64);
            return stream;
        }
    };
}

#endif //TOKEN_USER_H
