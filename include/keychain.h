#ifndef TOKEN_KEYCHAIN_H
#define TOKEN_KEYCHAIN_H

#include "common.h"

namespace Token{
    class Keychain{
    public:
        static const size_t kKeypairSize = 4096;
    private:
        Keychain() = delete;
    public:
        ~Keychain() = delete;

        static void Initialize();
        static void LoadKeys(CryptoPP::RSA::PrivateKey* privKey, CryptoPP::RSA::PublicKey* pubKey);
    };
}

#endif //TOKEN_KEYCHAIN_H