#ifndef TOKEN_USER_H
#define TOKEN_USER_H

#include "common.h"

namespace Token{
    class User{
    private:
        std::string userid_;
        CryptoPP::RSA::PublicKey public_key_;
        CryptoPP::RSA::PrivateKey private_key_;
    public:
        User(std::string userid, const std::string& pubkey, const std::string& privkey);
        User(std::string userid);

        std::string GetUserId() const{
            return userid_;
        }

        void Save(const std::string& pubkey, const std::string& privkey);

        static const size_t KEY_LENGTH = 4096;
    };
}

#endif //TOKEN_USER_H