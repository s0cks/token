#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>
#include "hash.h"

namespace Token{
    class Object;

#define FOR_EACH_TYPE(V) \
    V(Block) \
    V(Transaction) \
    V(Input) \
    V(Output) \
    V(UnclaimedTransaction) \
    V(BlockNode)         \
    V(Session)           \
    V(HttpSession)       \
    V(ServerSession)     \
    V(PeerSession)       \
    V(Buffer)            \
    V(Task)              \
    V(Message)           \
    V(Proposal)

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

    class Object{
        friend class Thread;
        friend class BinaryFileWriter;
    protected:
        Type type_;

        Object(Type type):
            type_(type){}
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

#define DECLARE_TYPECHECK(Name) \
        bool Is##Name() const{ \
            return GetType() == Type::k##Name##Type; \
        }
        FOR_EACH_TYPE(DECLARE_TYPECHECK)
#undef DECLARE_TYPECHECK
    };

    class Buffer;
    class BinaryObject : public Object{
    protected:
        BinaryObject(Type type):
            Object(type){}
    public:
        virtual ~BinaryObject() = default;

        Hash GetHash() const;
        virtual int64_t GetBufferSize() const = 0;
        virtual bool Encode(Buffer* buffer) const = 0;
    };

    class ObjectPointerVisitor{
    protected:
        ObjectPointerVisitor() = default;
    public:
        virtual ~ObjectPointerVisitor() = default;
        virtual bool Visit(Object* obj) = 0;
    };
}

#endif //TOKEN_OBJECT_H