#ifndef TOKEN_TOKEN_ID_H
#define TOKEN_TOKEN_ID_H

#include "common.h"
#include "byte_buffer.h"

namespace Token{
    class TokenID{
    public:
        static const size_t kSize = 64; // alias
    private:
        char data_[kSize];
    public:
        TokenID() = default;
        TokenID(const TokenID& token):
            data_(){
            memcpy(data_, token.data_, kSize);
        }
        TokenID(const std::string& value);
        TokenID(ByteBuffer* bytes);
        ~TokenID() = default;

        std::string Get() const;
        bool Encode(ByteBuffer* bytes) const;

        void operator=(const TokenID& token){
            memcpy(data_, token.data_, kSize);
        }

        friend bool operator==(const TokenID& a, const TokenID& b){
            return strncmp(a.data_, b.data_, 64) == 0;
        }

        friend bool operator==(const TokenID& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64)) == 0;
        }

        friend bool operator!=(const TokenID& a, const TokenID& b){
            return !operator==(a, b);
        }

        friend bool operator!=(const TokenID& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64)) != 0;
        }

        friend int operator<(const TokenID& a, const TokenID& b){
            return strncmp(a.data_, b.data_, 64);
        }

        friend int operator<(const TokenID& a, const std::string& b){
            return strncmp(a.data_, b.data(), std::min(b.length(), (unsigned long)64));
        }

        friend std::ostream& operator<<(std::ostream& stream, const TokenID& user){
            stream << std::string(user.data_, 64);
            return stream;
        }
    };
}

#endif //TOKEN_TOKEN_ID_H