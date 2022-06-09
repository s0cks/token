#ifndef TOKEN_SHA256_H
#define TOKEN_SHA256_H

#include <openssl/sha.h>
#include "token/uint256.h"

namespace token{
 namespace sha256{
  static const uint64_t kDigestSize = SHA256_DIGEST_LENGTH;
  static const uint64_t kSize = kDigestSize;

  static const uint64_t kDefaultNonceSize = 1024;

  uint256 Of(const uint8_t* data, size_t length);
  uint256 Nonce(uint64_t size = kDefaultNonceSize);
  uint256 Concat(const uint256& lhs, const uint256& rhs);
  uint256 FromHex(const char* data, size_t length);

  static inline uint256
  FromHex(const std::string& data){
    return FromHex(data.data(), data.length());
  }
 }
}

#endif//TOKEN_SHA256_H