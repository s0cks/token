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
    class Object;
    class ByteBuffer;

#define FOR_EACH_TYPE(V) \
    V(Block) \
    V(Transaction) \
    V(Input) \
    V(Output) \
    V(UnclaimedTransaction)

    enum class Type{
        kUnknownType=0,
#define DECLARE_TYPE(Name) k##Name##Type,
        FOR_EACH_TYPE(DECLARE_TYPE)
#undef DECLARE_TYPE
    };

    static std::ostream& operator<<(std::ostream& stream, const Type& type){
        switch(type){
#define DEFINE_TOSTRING(Name) \
            case Type::k##Name##Type: \
                stream << #Name; \
                return stream;
        FOR_EACH_TYPE(DEFINE_TOSTRING)
            case Type::kUnknownType:
            default:
                stream << "Unknown";
                return stream;
#undef DEFINE_TOSTRING
        }
    }

    typedef uint64_t ObjectHeader;

    ObjectHeader CreateObjectHeader(Object* obj);
    Type GetObjectHeaderType(ObjectHeader header);
    uint32_t GetObjectHeaderSize(ObjectHeader header);

    class Object : public RawObject{
        //TODO:
        // - track references using Reference class?
        // - create ObjectFile + ObjectBuffer classes for serialization purposes?
        friend class BinaryFileWriter;
    protected:
        Type type_;

        Object():
            RawObject(),
            type_(Type::kUnknownType){}
        Object(Type type):
            RawObject(),
            type_(type){}

        void SetType(Type type){
            type_ = type;
        }

        virtual bool Encode(ByteBuffer* bytes) const = 0;
    public:
        virtual ~Object() = default;

        Type GetType() const{
            return type_;
        }

        virtual std::string ToString() const{
            std::stringstream ss;
            ss << "Object(" << std::hex << this << ")";
            return ss.str();
        }

        virtual bool Equals(Object* obj) const{
            return GetHash() == obj->GetHash();
        }

        virtual bool Compare(Object* obj) const{
            return true;
        }

        bool WriteToFile(const std::string& filename) const{
            std::fstream fd(filename, std::ios::out|std::ios::binary);
            return WriteToFile(fd);
        }

        bool WriteToFile(std::fstream& file) const;
        uint256_t GetHash() const;
        virtual size_t GetBufferSize() const = 0; //TODO: refactor

#define DECLARE_TYPECHECK(Name) \
        bool Is##Name() const{ \
            return GetType() == Type::k##Name##Type; \
        }
        FOR_EACH_TYPE(DECLARE_TYPECHECK)
#undef DECLARE_TYPECHECK
    };

    class ObjectPointerVisitor{
    protected:
        ObjectPointerVisitor() = default;
    public:
        virtual ~ObjectPointerVisitor() = default;
        virtual bool Visit(RawObject* obj) = 0;
    };

    class ObjectPointerPrinter : public ObjectPointerVisitor{
    public:
        ObjectPointerPrinter() = default;
        ~ObjectPointerPrinter() = default;

        bool Visit(RawObject* obj){
            LOG(INFO) << "[" << std::hex << obj << ":" << obj->GetAllocatedSize() << "] (Marked: " << (obj->IsMarked() ? 'y' : 'n') << "): " << obj->ToString();
            return true;
        }
    };
}

#endif //TOKEN_OBJECT_H