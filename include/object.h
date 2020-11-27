#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>

#include "common.h"
#include "handle.h"
#include "keychain.h"
#include "allocator.h"
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
    V(Message)


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
        //TODO:
        // - track references using Reference class?
        // - create ObjectFile + ObjectBuffer classes for serialization purposes?

        friend class Thread;
        friend class Allocator;
        friend class Scavenger;
        friend class HandleGroup;
        friend class BinaryFileWriter;
        friend class ReferenceUpdater;
        friend class ReferenceNotifier;
        friend class StackReferenceNotifier;
    public:
        static const intptr_t kNumberOfCollectionsRequiredForPromotion = 3;
    protected:
        Type type_;
        bool marked_;
        intptr_t size_;
        intptr_t collections_survived_;
        intptr_t num_references_;
        uword ptr_;

        Object():
            type_(Type::kUnknownType),
            marked_(false),
            size_(0),
            collections_survived_(0),
            num_references_(0),
            ptr_(0){
            Allocator::Initialize(this);
        }

        uword GetStartAddress() const{
            return (uword)this;
        }

        uword GetEndAddress() const{
            return GetStartAddress() + GetSize();
        }

        void SetType(Type type){
            type_ = type;
        }

        void SetSize(intptr_t size){
            size_ = size;
        }

        void IncrementReferenceCount(){
            num_references_++;
        }

        void DecrementReferenceCount(){
            num_references_--;
        }

        void IncrementCollectionsCounter(){
            collections_survived_++;
        }

        void SetMarked(){
            marked_ = true;
        }

        void ClearMarked(){
            marked_ = false;
        }

        void WriteBarrier(Object** slot, Object* data){
            if(data) data->IncrementReferenceCount();
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

        virtual bool Accept(WeakObjectPointerVisitor* vis){ return true; }
    public:
        virtual ~Object() = default;

        Type GetType() const{
            return type_;
        }

        intptr_t GetSize() const{
            return size_;
        }

        intptr_t GetReferenceCount() const{
            return num_references_;
        }

        intptr_t GetNumberOfCollectionsSurvived() const{
            return collections_survived_;
        }

        bool IsForwarding() const{
            return ptr_ > 0;
        }

        bool HasStackReferences() const{
            return num_references_ > 0;
        }

        bool IsMarked() const{
            return marked_;
        }

        bool IsReadyForPromotion() const{
            return collections_survived_ >= kNumberOfCollectionsRequiredForPromotion;
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

        static void* operator new(size_t size){
            return Allocator::Allocate(size);
        }

        static void* operator new[](size_t) = delete;
        static void operator delete(void*){
            assert(0);
        }
    };

    class Buffer;
    class BinaryObject : public Object{
    public:
        BinaryObject(): Object(){}
        virtual ~BinaryObject() = default;

        Hash GetHash() const;
        virtual intptr_t GetBufferSize() const = 0;
        virtual bool Write(const Handle<Buffer>& buff) const = 0;
        virtual bool WriteToFile(std::fstream& fd) const;

        bool WriteToFile(const std::string& filename) const{
            std::fstream fd(filename, std::ios::binary|std::ios::out);
            return WriteToFile(fd);
        }
    };

    class ObjectPointerVisitor{
    protected:
        ObjectPointerVisitor() = default;
    public:
        virtual ~ObjectPointerVisitor() = default;
        virtual bool Visit(Object* obj) = 0;
    };

    class ObjectPointerPrinter : public ObjectPointerVisitor{
    public:
        ObjectPointerPrinter() = default;
        ~ObjectPointerPrinter() = default;

        bool Visit(Object* obj){
            LOG(INFO) << "[" << std::hex << obj << ":" << std::dec << obj->GetSize() << "] (Marked: " << (obj->IsMarked() ? 'y' : 'n') << "): " << obj->ToString();
            return true;
        }
    };
}

#endif //TOKEN_OBJECT_H