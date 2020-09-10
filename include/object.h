#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>

#include "common.h"
#include "handle.h"
#include "bitfield.h"
#include "allocator.h"
#include "uint256_t.h"

namespace Token{
    class Object;
    class ByteBuffer;

#define FOR_EACH_TYPE(V) \
    V(Input) \
    V(Output) \
    V(Transaction) \
    V(Block) \
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

    class WeakReferenceVisitor;
    class Object{
        //TODO:
        // - track references using Reference class?
        // - create ObjectFile + ObjectBuffer classes for serialization purposes?
        friend class Semispace;
        friend class Allocator;
        friend class Scavenger;
        friend class ObjectFinalizer;
        friend class ObjectRelocator;
        friend class ReferenceNotifier;
        friend class BinaryFileWriter;
    private:
        enum ObjectLayoutBits{
            kMarkedBit = 0,
            kReservedTagPosition = 1,
            kReservedTagSize = 2,

            kSizeFieldPosition = kReservedTagPosition+kReservedTagSize,
            kSizeFieldSize = 8,

            kTypeFieldPosition = kSizeFieldPosition+kSizeFieldSize,
            kTypeFieldSize = 16
        };

        class SizeBits : public BitField<uint32_t, intptr_t, kSizeFieldPosition, kSizeFieldSize>{};
        class TypeBits : public BitField<uint32_t, Type, kTypeFieldPosition, kTypeFieldSize>{};
        class ReservedBits : public BitField<uint32_t, intptr_t, kReservedTagPosition, kReservedTagSize>{};
        class MarkedBit : public BitField<uint32_t, bool, kMarkedBit, 1>{};

        uint32_t tag_;
        uint32_t num_references_; //TODO: remove Object::num_references_
        uword ptr_; //TODO: refactor

        void SetTag(uint32_t tag){
            tag_ = tag;
        }
    protected:
        Object():
            tag_(0){
            Allocator::Initialize(this);
            SetType(Type::kUnknownType);
        }

        uint32_t GetTag() const{
            return tag_;
        }

        void SetForwardingAddress(uword ptr){
            ptr_ = ptr;
        }

        void SetType(Type type){
            SetTag(TypeBits::Update(type, GetTag()));
        }

        void SetSize(intptr_t size){
            SetTag(SizeBits::Update(size, GetTag()));
        }

        void SetMarkedBit(){
            SetTag(MarkedBit::Update(true, GetTag()));
        }

        void ClearMarkedBit(){
            SetTag(MarkedBit::Update(false, GetTag()));
        }

        bool IsMarked() const{
            return MarkedBit::Decode(GetTag());
        }

        virtual bool Encode(ByteBuffer* bytes) const = 0;
        virtual bool Accept(WeakReferenceVisitor* vis){ return true; }

        void WriteBarrier(Object** slot, Object* data){
            if(data)data->IncrementReferenceCount();
            if((*slot))(*slot)->DecrementReferenceCount();
            (*slot) = data;
        }

        template<typename T, typename U>
        void WriteBarrier(T** slot, U* data){
            WriteBarrier((Object**)slot, (Object*)data);
        }

        template<typename T>
        void WriteBarrier(T** slot, const Handle<T>& handle){
            WriteBarrier((Object**)slot, (Object*)handle);
        }

        uint32_t GetReferenceCount() const{
            return num_references_;
        }

        bool HasStackReferences() const{
            return num_references_ > 0;
        }
    public:
        virtual ~Object() = default;

        Type GetType() const{
            return TypeBits::Decode(GetTag());
        }

        intptr_t GetSize() const{
            return SizeBits::Decode(GetTag());
        }

        uword GetStartAddress() const{
            return (uword)ptr_;
        }

        uword GetEndAddress() const{
            return GetStartAddress() + GetSize();
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

        void IncrementReferenceCount(){
            num_references_++;
        }

        void DecrementReferenceCount(){
            num_references_--;
        }

        bool WriteToFile(std::fstream& file) const;
        virtual uint256_t GetHash() const;
        virtual size_t GetBufferSize() const = 0; //TODO: refactor

#define DECLARE_TYPECHECK(Name) \
        bool Is##Name() const{ \
            return GetType() == Type::k##Name##Type; \
        }
        FOR_EACH_TYPE(DECLARE_TYPECHECK)
#undef DECLARE_TYPECHECK

        static void* operator new(size_t size){
            return Allocator::Allocate(size);
        }
    };

    class ObjectPointerVisitor{
    protected:
        ObjectPointerVisitor() = default;
    public:
        virtual ~ObjectPointerVisitor() = default;
        virtual bool Visit(Object* obj) = 0;
    };

    class ObjectPrinter : public ObjectPointerVisitor{
    public:
        ObjectPrinter() = default;
        ~ObjectPrinter() = default;

        bool Visit(Object* obj){
            LOG(INFO) << "[" << std::hex << (uword) obj << "] := " << obj->ToString();
            return true;
        }
    };
}

#endif //TOKEN_OBJECT_H