#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>

#include "common.h"
#include "uint256_t.h"
#include "keychain.h"

namespace Token{
    class HandleBase{
    private:
        Object** ptr_;
    protected:
        HandleBase(): ptr_(nullptr){}
        HandleBase(Object* obj);
        HandleBase(const HandleBase& h);

        Object* GetPointer() const{
            return ptr_ ? (*ptr_) : nullptr;
        }

        void operator=(Object* obj);
        void operator=(const HandleBase& h);
    };

    template<typename T>
    class Handle : HandleBase{
    public:
        Handle(): HandleBase(){}
        Handle(T* obj): HandleBase((Object*)obj){}
        Handle(const Handle& h): HandleBase(h){}

        void operator=(T* obj){
            HandleBase::operator=(obj);
        }

        void operator=(const Handle& h){
            HandleBase::operator=(h);
        }

        T* operator->() const{
            return static_cast<T*>(GetPointer());
        }

        operator T*() const{
            return reinterpret_cast<T*>(GetPointer());
        }

        template<typename U>
        Handle<U> CastTo() const{
            return static_cast<U*>(operator T*());
        }

        template<typename U>
        Handle<U> DynamicCastTo() const {
            return dynamic_cast<U*>(operator T*());
        }

        template<class S> friend class Handle;
    };

    class FieldIterator{
    public:
        virtual void operator()(Object** field) const = 0;

        template<typename T>
        void operator()(T** field) const{
            operator()(reinterpret_cast<Object**>(field));
        }
    };

    typedef uint64_t ObjectHeader;
    class Object{
        friend class StackSpaceIterator;
        friend class UpdateIterator;
        friend class WeakReferenceNotifyIterator;
        friend class Allocator;
    public:
        enum Color{
            kWhite=1,
            kGray,
            kBlack,
            kFree=kWhite,
            kRoot=kBlack,
        };

        //TODO: refactor typing system
        enum Type{
            kUnknown = 0,
            kBlock,
            kTransaction,
            kUnclaimedTransaction,
            kBlockNode,
            kMerkleNode,
            kBytes,
            kMessage,
        };

        enum Space{
            kStackSpace,
            kEdenSpace,
            kSurvivorSpace
        };
    protected:
        union{
            struct{
                Object* prev_;
                Object* next_;
            } stack_;
            struct{
                Object* ptr_;
                ObjectHeader header_;
                uint32_t refcount_;
            };
        };
        Space space_;

        inline void WriteBarrier(Object** slot, Object* data){
            //TODO: switch spaces
            (*slot) = data;
        }

        template<typename T, typename U>
        inline void WriteBarrier(T** slot, U* data){
            WriteBarrier(reinterpret_cast<Object**>(slot), static_cast<Object*>(static_cast<T*>(data)));
        }

        template<typename T>
        inline void WriteBarrier(T** slot, const Handle<T>& h){
            WriteBarrier(reinterpret_cast<Object**>(slot), static_cast<Object*>(h));
        }

        inline ObjectHeader
        GetHeader() const{
            return header_;
        }

        inline void
        SetHeader(ObjectHeader header){
            header_ = header;
        }

        inline void
        SetSpace(Space space){
            space_ = space;
        }

        inline void
        SetReferenceCount(uint32_t count){
            refcount_ = count;
        }

        virtual void NotifyWeakReferences(Object** field){}
    public:
        Object();
        virtual ~Object();

        virtual void Accept(const FieldIterator& iter){}

        void SetColor(Color color);
        void SetSize(uint32_t size);
        void SetType(Type type);
        Color GetColor() const;
        Type GetType() const;
        uint32_t GetSize() const;

        Space GetSpace() const{
            return space_;
        }

        size_t GetAllocatedSize() const{
            return GetSize();
        }

        uint32_t GetReferenceCount() const{
            return refcount_;
        }

        bool HasStackReferences() const{
            return GetReferenceCount() > 0;
        }

        bool IsGarbage() const{
            return GetColor() == kWhite;
        }

        void IncrementReferenceCount(){
            refcount_++;
        }

        void DecrementReferenceCount(){
            refcount_--;
        }

        static void* operator new(size_t);
        static void* operator new[](size_t) = delete;
        static void operator delete(void*){}
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

        bool WriteToFile(std::fstream &fd) const{
            RawObjectType raw;
            if(!Encode(raw)) return false;
            return raw.SerializeToOstream(&fd);
        }

        uint256_t GetSHA256Hash() const{
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

    class ArrayBase : public Object{
    private:
        size_t length_;
        Object* slots_[1];
    protected:
        static void* operator new(size_t size) = delete;
        static void* operator new(size_t size, size_t length, bool);
        static void operator delete(void*, size_t, bool);
        using Object::operator delete;

        ArrayBase(size_t length);

        void Put(size_t index, Object* obj){
            WriteBarrier(&slots_[index], obj);
        }

        Object* Get(size_t index){
            return slots_[index];
        }

        size_t Length(){
            return length_;
        }

        void Accept(const FieldIterator& it);
    };

    template<typename T>
    class Array : public ArrayBase{
    private:
        Array(size_t size): ArrayBase(size){}

        struct Iterator{
            Handle<Array<T>> self;
            size_t ptr;

            Handle<T> operator*() {
                return self->Get(ptr);
            }

            bool operator !=(const Iterator& another) {
                return ptr != another.ptr;
            }

            Iterator& operator++() {
                ptr++;
                return *this;
            }
        };

        struct Iterable {
            Handle<Array<T>> self;

            Iterator begin() {
                return{ self, 0 };
            }

            Iterator end() {
                return{ self, self->Length() };
            }
        };
    public:
        void Put(size_t index, const Handle<T>& obj){
            ArrayBase::Put(index, obj);
        }

        Handle<T> Get(size_t index){
            return static_cast<T*>(ArrayBase::Get(index));
        }

        size_t Length(){
            return ArrayBase::Length();
        }

        Iterable GetIterable(){
            return { this };
        }

        static Handle<Array> New(size_t length){
            return new (length, false)Array(length);
        }
    };
}

#endif //TOKEN_OBJECT_H