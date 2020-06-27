#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>

#include "common.h"
#include "uint256_t.h"
#include "bitfield.h"

namespace Token{
    class Object{
    public:
        virtual ~Object() = default;
        virtual std::string ToString() const = 0;

        virtual bool Finalize(){
            return true; //Implement in child-classes
        }
    };

    template<typename RawObjectType>
    class BinaryObject : public Object{
    protected:
        bool GetBytes(CryptoPP::SecByteBlock& bytes) const{
            RawObjectType raw;
            if(!Encode(raw)) return false;
            bytes.resize(raw.ByteSizeLong());
            return raw.SerializeToArray(bytes.data(), bytes.size());
        }
    public:
        virtual ~BinaryObject() = default;

        virtual bool Encode(RawObjectType& raw) const = 0;

        uint256_t GetHash() const{
            CryptoPP::SHA256 func;
            CryptoPP::SecByteBlock bytes;
            if(!GetBytes(bytes)) {
                LOG(WARNING) << "couldn't get bytes";
                return uint256_t();
            }
            CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
            CryptoPP::ArraySource source(bytes.data(), bytes.size(), true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
            return uint256_t(hash.data());
        }
    };
}

#endif //TOKEN_OBJECT_H