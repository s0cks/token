#ifndef TOKEN_COMMON_H
#define TOKEN_COMMON_H

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <sys/time.h>
#include <sys/stat.h>

#include <cryptopp/sha.h>
#include <cryptopp/hex.h>
#include <cryptopp/rsa.h>
#include <cryptopp/files.h>
#include <cryptopp/osrng.h>

namespace Token{
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

    typedef uint8_t Byte;

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

    static uint32_t
    GetCurrentTime(){
        struct timeval time;
        gettimeofday(&time, NULL);
        uint32_t curr_time = ((uint32_t)time.tv_sec * 1000 + time.tv_usec / 1000);
        return curr_time;
    }

    static inline std::string
    GenerateNonce(){
        CryptoPP::SecByteBlock nonce(64);
        CryptoPP::AutoSeededRandomPool prng;
        prng.GenerateBlock(nonce, nonce.size());

        std::string digest;
        CryptoPP::ArraySource source(nonce, nonce.size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest)));
        return digest;
    }

    static inline bool
    FileExists(const std::string& name){
        std::ifstream f(name.c_str());
        return f.good();
    }

    static inline bool
    CreateDirectory(const std::string& path){
        return (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) != -1;
    }
}

#endif //TOKEN_COMMON_H