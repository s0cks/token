#ifndef TOKEN_KEYCHAIN_H
#define TOKEN_KEYCHAIN_H

#include "common.h"
#include "crypto.h"

namespace token{
  class Keychain{
   public:
    static const size_t kKeypairSize = 4096;
   public:
    Keychain() = delete;
    ~Keychain() = delete;

    static bool Initialize();
    static bool LoadKeys(CryptoPP::RSA::PrivateKey* privKey, CryptoPP::RSA::PublicKey* pubKey);

    static inline const char*
    GetName(){
      return "Keychain";
    }
  };
}

#endif //TOKEN_KEYCHAIN_H