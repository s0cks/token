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
    class FieldIterator{
    public:
        virtual void operator()(Object** field) const = 0;

        template<typename T>
        void operator()(T** field) const{
            operator()(reinterpret_cast<Object**>(field));
        }
    };

    typedef uint64_t ObjectHeader;

    //TODO:
    // - add GC debug information
    class Object{
        friend class UpdateIterator;

        friend class Allocator;
        friend class Scavenger;
        friend class RootPage;
        friend class ObjectFinalizer;
        friend class ObjectRelocator;
        friend class ObjectPromoter;
        friend class LiveObjectMarker;
        friend class LiveObjectCopier;
        friend class LiveObjectReferenceUpdater;
        friend class RootObjectReferenceUpdater;
    public:
        enum Color{
            kWhite=1,
            kGray,
            kBlack,
            kFree=kWhite,
            kRoot=kBlack,
        };

        // Expected Object Layout:
        //   | Size - 32 bits
        //   | Color - 8 bits
        //   | Reference Count - 32 bits
        //   | Ptr - 64 bits
    protected:
        ObjectHeader header_;
        Object* ptr_;//TODO: forwarding address
        uint32_t refcount_;
        GCStats stats_;

        inline void WriteBarrier(Object** slot, Object* data){
            if(data) data->IncrementReferenceCount();
            if((*slot)) (*slot)->DecrementReferenceCount();
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

        GCStats* GetStats(){
            return &stats_;
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

        size_t GetAllocatedSize() const{
            return GetSize(); // refactor
        }

        bool HasStackReferences() const{
            return GetReferenceCount() > 0;
        }

        bool IsGarbage() const{
            return GetColor() == kWhite;
        }

        bool IsReadyForPromotion(){
            return stats_.GetNumberOfCollectionsSurvived() >= GCStats::kNumberOfCollectionsRequiredForPromotion;
        }

        virtual std::string ToString() const{
            std::stringstream ss;
            ss << "Object(" << std::hex << this << ")";
            return ss.str();
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

    class ObjectPointerVisitor{
    protected:
        ObjectPointerVisitor() = default;
    public:
        virtual ~ObjectPointerVisitor() = default;
        virtual bool Visit(Object* obj) = 0;
    };
}

#endif //TOKEN_OBJECT_H