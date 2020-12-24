#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>
#include "hash.h"

namespace Token{
    class Object;

    class Object{
        friend class Thread;
        friend class BinaryFileWriter;
    public:
        virtual ~Object() = default;
        virtual std::string ToString() const = 0;
    };

    class Buffer;
    class BinaryObject : public Object{
    protected:
        BinaryObject() = default;
    public:
        virtual ~BinaryObject() = default;

        Hash GetHash() const;
        virtual int64_t GetBufferSize() const = 0;
        virtual bool Encode(Buffer* buffer) const = 0;
    };
}

#endif //TOKEN_OBJECT_H