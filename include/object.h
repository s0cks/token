#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>

#include "common.h"
#include "uint256_t.h"
#include "keychain.h"
#include "handle.h"

namespace Token{
    class FieldIterator{
    public:
        virtual void operator()(Object** field) const = 0;

        template<typename T>
        void operator()(T** field) const{
            operator()(reinterpret_cast<Object**>(field));
        }
    };

#define FOR_EACH_OBJECT_TYPE(V) \
    V(Transaction) \
    V(Block)

    template<typename T>
    class Array;

    class Instance;
#define FORWARD_DECLARE_OBJECT_TYPE(Name) class Name;
    FOR_EACH_OBJECT_TYPE(FORWARD_DECLARE_OBJECT_TYPE);
#undef FORWARD_DECLARE_OBJECT_TYPE

    enum class Type{
        kUnknownType = 0,
#define DECLARE_OBJECT_TYPE(Name) k##Name##Type,
    FOR_EACH_OBJECT_TYPE(DECLARE_OBJECT_TYPE)
#undef DECLARE_OBJECT_TYPE
        kArrayType,
    };

    typedef uint64_t ObjectHeader;

    //TODO:
    // - add GC debug information
    class Object{
        friend class Allocator;
        friend class HandleGroup;
        friend class UpdateIterator;
        friend class IncRefIterator;
        friend class DecRefIterator;
        friend class StackSpaceIterator;
        friend class WeakReferenceNotifyIterator;
    public:
        enum Color{
            kWhite=1,
            kGray,
            kBlack,
            kFree=kWhite,
            kRoot=kBlack,
        };

        enum Space{
            kStackSpace,
            kEdenSpace,
            kSurvivorSpace
        };

        // Expected Object Layout:
        //   | Type - 16 bits
        //   | Size - 32 bits
        //   | Color - 8 bits
        //   | Reference Count - 32 bits
        //   | Ptr - 64 bits
    protected:
        ObjectHeader header_;
        union{ // layout is weird
            struct{
                Object* prev_;
                Object* next_;
            } stack_;
            struct{
                Object* ptr_;
                uint32_t refcount_;
            };
        };
        Space space_; // remove?

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

        inline void
        IncrementReferenceCount(){
            refcount_++;
        }

        inline void
        DecrementReferenceCount(){
            refcount_--;
        }

        void SetColor(Color color);
        void SetSize(uint32_t size);
        virtual void NotifyWeakReferences(Object** field){}
        virtual void Accept(const FieldIterator& iter){}
    public:
        Object();
        virtual ~Object();

        uint32_t GetSize() const;
        Color GetColor() const;

        uint32_t GetReferenceCount() const{
            return refcount_;
        }

        Space GetSpace() const{
            return space_;
        }

        size_t GetAllocatedSize() const{
            return GetSize(); // refactor
        }

        bool HasStackReferences() const{
            return GetReferenceCount() > 0;
        }

        bool IsGarbage() const{
            return GetColor() == kWhite;
        }

        static void* operator new(size_t);
        static void* operator new[](size_t) = delete;
        static void operator delete(void*){}
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
}

#endif //TOKEN_OBJECT_H