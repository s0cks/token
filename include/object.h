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
    class Object : public RawObject{
        //TODO:
        // - track references using Reference class?
    protected:
        Object(): RawObject(){}

        virtual size_t GetBufferSize() const = 0;
        virtual bool Encode(uint8_t* bytes) const = 0;

        bool Encode(CryptoPP::SecByteBlock& bytes){
            size_t size = GetBufferSize();
            bytes.resize(size);
            Encode(bytes.data());
            return true;
        }
    public:
        virtual std::string ToString() const{
            std::stringstream ss;
            ss << "Object(" << std::hex << this << ")";
            return ss.str();
        }

        bool WriteToFile(std::fstream& file) const{
            size_t size = GetBufferSize();
            uint8_t bytes[size];
            if(!Encode(bytes)){
                LOG(WARNING) << "couldn't encode object to bytes";
                return false;
            }
            file.write((char*)bytes, size);
            file.flush();
            return true;
        }

        bool WriteToFile(const std::string& filename) const{
            std::fstream fd(filename, std::ios::out|std::ios::binary);
            return WriteToFile(fd);
        }

        uint256_t GetHash() const{
            size_t size = GetBufferSize();
            CryptoPP::SHA256 func;
            CryptoPP::SecByteBlock bytes(size);
            if(!Encode(bytes)){
                LOG(WARNING) << "couldn't encode object to bytes";
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