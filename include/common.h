#ifndef TOKEN_COMMON_H
#define TOKEN_COMMON_H

#include <cstdint>
#include <cstdlib>
#include <vector>

#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/rsa.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>

namespace Token{
    typedef std::vector<char> Buffer;

#if defined(_M_X64) || defined(__x86_64__)
#define ARCHITECTURE_IS_X64 1
#elif defined(_M_IX86) || defined(__i386__)
#define ARCHITECTURE_IS_X32 1
#elif defined(__ARMEL__)
#define ARCHITECTURE_IS_ARM 1
#elif defined(__aarch64__)
#define ARCHITECTURE_IS_ARM64 1
#else
#error "Cannot determine CPU architecture"
#endif

    typedef byte Byte;

    static inline uintptr_t
    RoundUpPowTwo(uintptr_t x){
        x = x - 1;
        x = x | (x >> 1);
        x = x | (x >> 2);
        x = x | (x >> 4);
        x = x | (x >> 8);
        x = x | (x >> 16);
#if defined(ARCHITECTURE_IS_X64)
        x = x | (x >> 32);
#endif
        return x + 1;
    }

    namespace Bytes{
        static inline Byte*
        ArrayNew(size_t size){
            Byte* bytes = reinterpret_cast<Byte*>(malloc(sizeof(Byte) * size));
            return bytes;
        }

        static inline size_t
        ArrayLength(Byte* bytes){
            return sizeof(bytes) / sizeof(Byte);
        }

        static inline int
        ArrayCompare(Byte* bytes1, Byte* bytes2){
            if(ArrayLength(bytes1) > ArrayLength(bytes2)) return 1;
            else if(ArrayLength(bytes1) < ArrayLength(bytes2)) return -1;
            return std::memcmp(bytes1, bytes2, ArrayLength(bytes1));
        }

        static inline uint32_t
        ArrayHashCode(Byte* bytes){
            uint32_t hash = 1;
            for(size_t i = 0; i < ArrayLength(bytes); i++) hash = 31 * hash + bytes[i];
            return hash;
        }
    }

    static inline std::string
    Hash(const std::string& value){
        std::string digest;
        CryptoPP::SHA256 hash;
        CryptoPP::StringSource source(value, true, new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    static inline std::string
    Hash(Byte* value){
        std::string digest;
        CryptoPP::SHA256 hash;
        CryptoPP::ArraySource source(value, sizeof(value), true, new CryptoPP::HashFilter(hash, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }
}

#endif //TOKEN_COMMON_H