#ifndef TOKEN_HASH_H
#define TOKEN_HASH_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "common.h"

namespace Token{
    class Hash{
    public:
        static const intptr_t kSize = 256 / 8;
    private:
        uint8_t data_[kSize];

        Hash(const unsigned char* data):
            data_(){
            memcpy(data_, data, sizeof(data_));
        }

        Hash(const std::string& hex):
            data_(){
            CryptoPP::StringSource source(hex, true, new CryptoPP::HexDecoder(new CryptoPP::ArraySink(data_, kSize)));
        }

        Hash(const Hash& first, const Hash& second):
            data_(){
            size_t size = first.size() + second.size();
            uint8_t bytes[size];
            memcpy(&bytes[0], first.data(), first.size());
            memcpy(&bytes[first.size()], second.data(), second.size());
            CryptoPP::SHA256 func;
            CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(data_, kSize)));
        }
    public:
        Hash():
            data_(){
            SetNull();
        }
        Hash(uint8_t* data):
            data_(){
            memcpy(data_, data, kSize);
        }
        Hash(const Hash& hash):
            data_(){
            memcpy(data_, hash.data_, kSize);
        }
        ~Hash() = default;

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

        inline int Compare(const Hash& other) const{
            return memcmp(data_, other.data_, sizeof(data_));
        }

        friend inline bool operator==(const Hash& a, const Hash& b){
            return a.Compare(b) == 0;
        }

        friend inline bool operator!=(const Hash& a, const Hash& b){
            return a.Compare(b) != 0;
        }

        friend inline bool operator<(const Hash& a, const Hash& b){
            return a.Compare(b) < 0;
        }

        void operator=(const Hash& other){
            memcpy(data_, other.data_, sizeof(data_));
        }

        static Hash
        GenerateNonce(){
            CryptoPP::SecByteBlock nonce(kSize);
            CryptoPP::AutoSeededRandomPool prng;
            prng.GenerateBlock(nonce, nonce.size());
            uint8_t data[kSize];
            CryptoPP::ArraySource source(nonce, nonce.size(), true, new CryptoPP::HexEncoder(new CryptoPP::ArraySink(data, kSize)));
            return Hash(data);
        }

        static Hash
        FromBytes(uint8_t* bytes){
            return Hash(bytes);
        }

        static Hash
        FromHexString(const std::string& hex){
            return Hash(hex);
        }

        static Hash
        Concat(const Hash& first, const Hash& second){
            return Hash(first, second);
        }

        friend std::ostream& operator<<(std::ostream& stream, const Hash& hash){
            stream << hash.HexString();
            return stream;
        }
    };
}

#endif //TOKEN_HASH_H