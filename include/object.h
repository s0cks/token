#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>

#include "common.h"
#include "allocator.h"
#include "uint256_t.h"
#include "keychain.h"
#include "handle.h"

namespace Token{
    //TODO: cleanup class + memory layout
    //TODO: track references using Reference class?
    class Object : public RawObject{
    protected:
        Object(): RawObject(){}
    public:
        virtual ~Object() = default;

        virtual std::string ToString() const{
            std::stringstream ss;
            ss << "Object(" << std::hex << this << ")";
            return ss.str();
        }
    };

    template<typename RawType>
    class BinaryObject : public Object{
    protected:
        BinaryObject(): Object(){}
    public:
        ~BinaryObject(){}

        virtual bool WriteToMessage(RawType& raw) const = 0;

        bool WriteToFile(std::fstream& fd) const{
            RawType msg;
            if(!WriteToMessage(msg)) return false;
            return msg.SerializeToOstream(&fd);
        }

        bool WriteToBytes(CryptoPP::SecByteBlock& bytes) const{
            RawType raw;
            if(!WriteToMessage(raw)) return false;
            bytes.resize(raw.ByteSizeLong());
            return raw.SerializeToArray(bytes.data(), bytes.size());
        }

        uint256_t GetSHA256Hash() const{
            CryptoPP::SHA256 func;
            CryptoPP::SecByteBlock bytes;
            if(!WriteToBytes(bytes)) {
                LOG(WARNING) << "couldn't get bytes";
                return uint256_t();
            }
            CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
            CryptoPP::ArraySource source(bytes.data(), bytes.size(), true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
            return uint256_t(hash.data());
        }
    };

    class ObjectPointerVisitor{
    protected:
        ObjectPointerVisitor() = default;
    public:
        virtual ~ObjectPointerVisitor() = default;
        virtual bool Visit(RawObject* obj) = 0;
    };
}

#endif //TOKEN_OBJECT_H