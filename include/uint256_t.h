#ifndef TOKEN_UINT256_T_H
#define TOKEN_UINT256_T_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "common.h"

namespace Token{
    class uint256_t{
    public:
        static const size_t kSize = 256 / 8;
    private:
        uint8_t data_[kSize];

        uint256_t():
            data_(){
            SetNull();
        }
        uint256_t(const unsigned char* data):
            data_(){
            memcpy(data_, data, sizeof(data_));
        }
        uint256_t(uint8_t* data):
            data_(){
            memcpy(data_, data, sizeof(data_));
        }

        uint256_t(const std::string& hex):
            data_(){
            CryptoPP::StringSource source(hex, true, new CryptoPP::HexDecoder(new CryptoPP::ArraySink(data_, kSize)));
        }

        uint256_t(const uint256_t& first, const uint256_t& second):
            data_(){
            size_t size = first.size() + second.size();
            uint8_t bytes[size];
            memcpy(&bytes[0], first.data(), first.size());
            memcpy(&bytes[first.size()], second.data(), second.size());
            CryptoPP::SHA256 func;
            CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(data_, kSize)));
        }
    public:
        uint256_t(const uint256_t& hash):
            data_(){
            memcpy(data_, hash.data_, kSize);
        }

        void SetNull(){
            memset(data_, 0, sizeof(data_));
        }

        bool IsNull() const{
            for(size_t idx = 0; idx < kSize; idx++){
                if(data_[idx] != 0) return false;
            }
            return true;
        }

        unsigned long GetHashCode() const{
            uint64_t a = ((uint64_t*)data_)[0];
            uint64_t b = ((uint64_t*)data_)[1];
            uint64_t c = ((uint64_t*)data_)[2];
            uint64_t d = ((uint64_t*)data_)[3];
            unsigned long h = 0;
            h = 31 * h + a;
            h = 31 * h + b;
            h = 31 * h + c;
            h = 31 * h + d;
            return h;
        }

        unsigned char* data() const{
            return (unsigned char*)data_;
        }

        size_t size() const{
            return kSize;
        }

        std::string HexString() const{
            std::string hash;
            CryptoPP::ArraySource source(data(), size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash)));
            return hash;
        }

        inline int Compare(const uint256_t& other) const{
            return memcmp(data_, other.data_, sizeof(data_));
        }

        friend inline bool operator==(const uint256_t& a, const uint256_t& b){
            return a.Compare(b) == 0;
        }

        friend inline bool operator!=(const uint256_t& a, const uint256_t& b){
            return a.Compare(b) != 0;
        }

        friend inline bool operator<(const uint256_t& a, const uint256_t& b){
            return a.Compare(b) < 0;
        }

        void operator=(const uint256_t& other){
            memcpy(data_, other.data_, sizeof(data_));
        }

        static uint256_t
        Null(){
            return uint256_t();
        }

        static uint256_t
        FromBytes(uint8_t* bytes){
            return uint256_t(bytes);
        }

        static uint256_t
        FromHexString(const std::string& hex){
            return uint256_t(hex);
        }

        static uint256_t
        Concat(const uint256_t& first, const uint256_t& second){
            return uint256_t(first, second);
        }

        friend std::ostream& operator<<(std::ostream& stream, const uint256_t& hash){
            stream << hash.HexString();
            return stream;
        }
    };
}

#endif //TOKEN_UINT256_T_H