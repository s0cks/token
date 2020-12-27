#ifndef TOKEN_HASH_H
#define TOKEN_HASH_H

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <unordered_set>
#include <rapidjson/document.h>
#include "common.h"

namespace Token{
    class Hash{
    private:
        static const int64_t kSize = 256 / 8;
    public:
        struct Hasher{
        public:
            size_t operator()(const Hash& hash) const{
                uint64_t a = ((uint64_t*)hash.data())[0];
                uint64_t b = ((uint64_t*)hash.data())[1];
                uint64_t c = ((uint64_t*)hash.data())[2];
                uint64_t d = ((uint64_t*)hash.data())[3];

                size_t res = 17;
                res = res * 31 + std::hash<uint64_t>()(a);
                res = res * 31 + std::hash<uint64_t>()(b);
                res = res * 31 + std::hash<uint64_t>()(c);
                res = res * 31 + std::hash<uint64_t>()(d);
                return res;
            }
        };

        struct Equal{
        public:
            bool operator()(const Hash& a, const Hash& b) const{
                return a.Compare(b) == 0;
            }
        };
    private:
        uint8_t data_[kSize];

        Hash(const unsigned char* data):
            data_(){
            memcpy(data_, data, sizeof(data_));
        }

        Hash(const std::string& hex):
            data_(){
            CryptoPP::StringSource source(hex, true, new CryptoPP::HexDecoder(new CryptoPP::ArraySink(data_, GetSize())));
        }

        Hash(const Hash& first, const Hash& second):
            data_(){
            size_t size = first.size() + second.size();
            uint8_t bytes[size];
            memcpy(&bytes[0], first.data(), first.size());
            memcpy(&bytes[first.size()], second.data(), second.size());
            CryptoPP::SHA256 func;
            CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(data_, GetSize())));
        }
    public:
        Hash():
            data_(){
            SetNull();
        }
        Hash(uint8_t* data, int64_t size):
            data_(){
            memcpy(data_, data, std::min(size, Hash::GetSize()));
        }
        Hash(const Hash& hash):
            data_(){
            memcpy(data_, hash.data_, GetSize());
        }
        ~Hash() = default;

        void SetNull(){
            memset(data_, 0, sizeof(data_));
        }

        bool IsNull() const{
            for(int64_t idx = 0; idx < GetSize(); idx++){
                if(data_[idx] != 0)
                    return false;
            }
            return true;
        }

        unsigned char* data() const{
            return (unsigned char*)data_;
        }

        int64_t size() const{
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

        friend std::ostream& operator<<(std::ostream& stream, const Hash& hash){
            stream << hash.HexString();
            return stream;
        }

        static inline int64_t
        GetSize(){
            return kSize;
        }

        static inline Hash
        FromHexString(const std::string& hex){
            return Hash(hex);
        }

        static inline Hash
        Concat(const Hash& first, const Hash& second){
            return Hash(first, second);
        }

        static inline Hash
        GenerateNonce(){
            CryptoPP::SecByteBlock nonce(GetSize());
            CryptoPP::AutoSeededRandomPool prng;
            prng.GenerateBlock(nonce, nonce.size());
            uint8_t data[GetSize()];
            CryptoPP::ArraySource source(nonce, nonce.size(), true, new CryptoPP::HexEncoder(new CryptoPP::ArraySink(data, GetSize())));
            return Hash(data);
        }
    };

    typedef std::unordered_set<Hash, Hash::Hasher, Hash::Equal> HashList;
}

#endif //TOKEN_HASH_H