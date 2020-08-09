#ifndef TOKEN_BYTES_H
#define TOKEN_BYTES_H

#include <cstdint>
#include <cstring>
#include <string>
#include "uint256_t.h"

namespace Token{
    static inline void
    EncodeInt(uint8_t* bytes, uint32_t value){
        memcpy(bytes, (uint8_t*)&value, sizeof(uint32_t));
    }

    static inline uint32_t
    DecodeInt(uint8_t* bytes){
        uint32_t result = 0;
        memcpy(&result, bytes, sizeof(uint32_t));
        return result;
    }

    static inline void
    EncodeString(uint8_t* bytes, const std::string& value){
        EncodeInt(&bytes[0], (uint32_t)value.length());
        memcpy(&bytes[4], value.data(), value.length());
    }

    static inline std::string
    DecodeString(uint8_t* bytes, uint32_t length){
        return std::string((char*)bytes, length);
    }

    static inline void
    EncodeHash(uint8_t* bytes, const uint256_t& hash){
        memcpy(bytes, hash.data(), uint256_t::kSize);
    }

    static inline uint256_t
    DecodeHash(uint8_t* bytes){
        uint256_t hash;
        memcpy(hash.data(), bytes, uint256_t::kSize);
        return hash;
    }
}

#endif //TOKEN_BYTES_H