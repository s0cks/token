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

    typedef CryptoPP::byte Byte;

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
}

#endif //TOKEN_COMMON_H