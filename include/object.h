#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>

#include "common.h"
#include "handle.h"
#include "keychain.h"
#include "allocator.h"
#include "uint256_t.h"

namespace Token{
    class ByteBuffer;
    class Object : public RawObject{
        //TODO:
        // - track references using Reference class?
    protected:
        Object(): RawObject(){}

        virtual size_t GetBufferSize() const = 0; //TODO: refactor
        virtual bool Encode(ByteBuffer* bytes) const = 0;
    public:
        virtual std::string ToString() const{
            std::stringstream ss;
            ss << "Object(" << std::hex << this << ")";
            return ss.str();
        }

        bool WriteToFile(const std::string& filename) const{
            std::fstream fd(filename, std::ios::out|std::ios::binary);
            return WriteToFile(fd);
        }

        bool WriteToFile(std::fstream& file) const;
        uint256_t GetHash() const;
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