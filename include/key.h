#ifndef TOKEN_KEY_H
#define TOKEN_KEY_H

#include "common.h"
#include "key.h"

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

#endif //TOKEN_KEY_H