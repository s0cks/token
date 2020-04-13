#ifndef TOKEN_BINARY_OBJECT_H
#define TOKEN_BINARY_OBJECT_H

#include "common.h"
#include "uint256_t.h"

namespace Token{
    class BinaryObject{
    protected:
        virtual bool GetBytes(CryptoPP::SecByteBlock& bytes) const = 0;
    public:
        virtual ~BinaryObject() = default;

        uint256_t GetHash() const{
            CryptoPP::SHA256 func;
            CryptoPP::SecByteBlock bytes;
            if(!GetBytes(bytes)) return uint256_t();
            CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
            CryptoPP::ArraySource source(bytes.data(), bytes.size(), true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
            return uint256_t(hash.data());
        }
    };
}

#endif //TOKEN_BINARY_OBJECT_H