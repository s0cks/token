#include "token/hash.h"

namespace token{
 const uint64_t sha256::kDigestSize = CryptoPP::SHA256::DIGESTSIZE;
 const uint64_t sha256::kSize = sha256::kDigestSize;
}