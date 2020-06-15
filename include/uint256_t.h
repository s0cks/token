#ifndef TOKEN_UINT256_T_H
#define TOKEN_UINT256_T_H

#include "common.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace Token{
    class uint256_t{
    public:
        static const size_t kSize = 256 / 8;
    private:
        uint8_t data_[kSize];
    public:
        uint256_t():
            data_(){
            SetNull();
        }
        uint256_t(const unsigned char* data):
            data_(){
            memcpy(data_, data, sizeof(data_));
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

        unsigned char* data() const{
            return (unsigned char*)data_;
        }

        size_t size() const{
            return kSize;
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

        uint256_t& operator=(const uint256_t& other){
            memcpy(data_, other.data_, sizeof(data_));
            return (*this);
        }
    };

    static inline std::string
    HexString(uint256_t val){
        std::string hash;
        CryptoPP::ArraySource source(val.data(), val.size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(hash)));
        return hash;
    }

    static inline uint256_t
    HashFromHexString(const std::string& val){
        uint256_t hash;
        CryptoPP::StringSource source(val, true, new CryptoPP::HexDecoder(new CryptoPP::ArraySink(hash.data(), hash.size())));
        return hash;
    }

    static inline uint256_t
    ConcatHashes(const uint256_t& first, const uint256_t& second){
        size_t size = first.size() + second.size();
        uint8_t bytes[size];
        memcpy(&bytes[0], first.data(), first.size());
        memcpy(&bytes[first.size()], second.data(), second.size());
        uint256_t hash;
        CryptoPP::SHA256 func;
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
        return hash;
    }

    static std::ostream& operator<<(std::ostream& stream, const uint256_t& val){
        stream << HexString(val);
        return stream;
    }
}

#endif //TOKEN_UINT256_T_H