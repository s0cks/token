#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <map>
#include <vector>
#include <string>

#include "common.h"
#include "uint256_t.h"
#include "bitfield.h"

namespace Token{
    enum Color{
        kFree = 1 << 0,
        kWhite = kFree,
        kGray = 1 << 1,
        kBlack = 1 << 2,
    };

    class RawObject;
    class ObjectPointerVisitor{
    public:
        virtual ~ObjectPointerVisitor() = default;
        virtual bool Visit(RawObject* obj) = 0;
    };

#define FOR_EACH_REFERENCE_TYPE(V) \
    V(Weak) \
    V(Strong)

    class Reference;
#define FORWARD_DECLARE_REFERENCE_CLASS(Name) class Name##Reference;
    FOR_EACH_REFERENCE_TYPE(FORWARD_DECLARE_REFERENCE_CLASS);
#undef FORWARD_DECLARE_REFERENCE_CLASS

    class Reference{
    private:
        RawObject* owner_;
        RawObject* target_;
        void** ptr_;
    public:
        Reference(RawObject* owner, RawObject* target, void** ptr):
                owner_(owner),
                target_(target),
                ptr_(ptr){}
        ~Reference(){}

        void** GetLocation() const{
            return ptr_;
        }

        RawObject* GetOwner() const{
            return owner_;
        }

        RawObject* GetTarget() const{
            return target_;
        }

#define DECLARE_TYPE_CAST(Name) \
    virtual Name##Reference* As##Name##Reference() { return nullptr; } \
    bool Is##Name##Reference() { return As##Name##Reference() != nullptr; }
        FOR_EACH_REFERENCE_TYPE(DECLARE_TYPE_CAST);
#undef DECLARE_TYPE_CAST
    };

#define DEFINE_REFERENCE_TYPE(Name) \
    public: \
        Name##Reference* As##Name##Reference() { return this; }

    class WeakReference : public Reference{
    public:
        WeakReference(RawObject* owner, RawObject* target, void** ptr): Reference(owner, target, ptr){}
        ~WeakReference(){}

        DEFINE_REFERENCE_TYPE(Weak);
    };

    class StrongReference : public Reference{
    public:
        StrongReference(RawObject* owner, RawObject* target, void** ptr):
                Reference(owner, target, ptr){}
        ~StrongReference(){}

        DEFINE_REFERENCE_TYPE(Strong);
    };

    class Object;
    class RawObject{
    public:
        typedef std::vector<std::unique_ptr<Reference>> ReferenceList;
    private:
        enum{
            kBitsForColor = 8,
            kBitsForCondemned = 1,
            kBitsForForwarding = 1,
            kBitsForSize = 32,
        };

        class ColorField : public BitField<uintptr_t, Color, 0, kBitsForColor>{};
        class CondemnedField : public BitField<uintptr_t, bool, kBitsForColor, kBitsForCondemned>{};
        class ForwardingField : public BitField<uintptr_t, bool, kBitsForColor + kBitsForCondemned, kBitsForForwarding>{};
        class SizeField : public BitField<uintptr_t, uint32_t, kBitsForColor + kBitsForCondemned + kBitsForForwarding, kBitsForSize>{};

        uintptr_t header_;
        void* ptr_;
        uintptr_t forwarding_address_;
        ReferenceList pointing_;
        ReferenceList owned_;

        void SetForwarding(bool val){
            header_ = ForwardingField::Update(val, header_);
        }

        void SetCondemned(bool val){
            header_ = CondemnedField::Update(val, header_);
        }

        void SetSize(uint32_t size){
            header_ = SizeField::Update(size, header_);
        }

        void SetColor(Color color){
            header_ = ColorField::Update(color, header_);
        }

        void SetForwardingAddress(uintptr_t address){
            if(IsForwarding()) return;
            SetForwarding(true);
            forwarding_address_ = address;
        }

        friend class Allocator;
        friend class Scavenger;
        friend class Reference;
        friend class WeakReference;
        friend class StrongReference;
        friend class GCMarker;
    public:
        RawObject(): RawObject(nullptr, 0){}
        RawObject(void* ptr, size_t size);
        ~RawObject(){}

        uintptr_t GetForwardingAddress() const{
            return forwarding_address_;
        }

        uintptr_t GetObjectAddress() const{
            return (uintptr_t)ptr_;
        }

        void* GetObjectPointer() const{
            return ptr_;
        }

        size_t GetObjectSize() const{
            return SizeField::Decode(header_);
        }

        bool IsCondemned() const{
            return CondemnedField::Decode(header_);
        }

        bool IsReferenced() const{
            return !pointing_.empty();
        }

        bool IsForwarding() const{
            return ForwardingField::Decode(header_);
        }

        Color GetColor() const{
            return ColorField::Decode(header_);
        }

        Object* GetObject() const{
            return (Object*)GetObjectPointer();
        }

        std::string ToString() const;
        bool VisitOwnedReferences(ObjectPointerVisitor* vis);
        bool VisitPointingReferences(ObjectPointerVisitor* vis);

        friend std::ostream& operator<<(std::ostream& stream, const RawObject& obj){
            stream << std::hex << obj.GetObjectPointer();
            stream << "(" << std::dec << obj.GetObjectSize() << " Bytes,";
            switch(obj.GetColor()){
                case Color::kFree:
                    stream << " Free";
                    break;
                case Color::kGray:
                    stream << " Gray";
                    break;
                case Color::kBlack:
                    stream << " Black";
                    break;
                default:
                    stream << " Unknown";
                    break;
            }
            stream << ")";
            return stream;
        }
    };

    class Object{
    public:
        virtual ~Object() = default;
        virtual std::string ToString() const = 0;
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

        uint256_t GetHash() const{
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
}

#endif //TOKEN_OBJECT_H