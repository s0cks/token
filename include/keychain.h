#ifndef TOKEN_KEYCHAIN_H
#define TOKEN_KEYCHAIN_H

#include "common.h"

namespace Token{
    class TokenKeychain{
    private:
        TokenKeychain(){}
    public:
        ~TokenKeychain(){}

        static bool InitializeKeys();
        static bool LoadKeys(CryptoPP::RSA::PrivateKey* privKey, CryptoPP::RSA::PublicKey* pubKey);
        static const size_t kKeypairSize = 4096;
    };
}

#endif //TOKEN_KEYCHAIN_H