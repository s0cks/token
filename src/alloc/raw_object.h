#ifndef TOKEN_RAW_OBJECT_H
#define TOKEN_RAW_OBJECT_H

#include <memory>
#include <vector>
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

    class Object;
    class RawObject{
    public:
        typedef std::vector<Reference*> ReferenceList;
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
        uintptr_t forwarding_address_;
        ReferenceList pointing_;
        ReferenceList owned_;
        void* ptr_;

        void SetHeader(uintptr_t value){
            header_ = value;
        }

        uintptr_t GetHeader(){
            return header_;
        }

        void SetForwarding(bool val){
            SetHeader(ForwardingField::Update(val, GetHeader()));
        }

        void SetCondemned(bool val){
            SetHeader(CondemnedField::Update(val, GetHeader()));
        }

        void SetSize(uint32_t val){
            SetHeader(SizeField::Update(val, GetHeader()));
        }

        void SetColor(Color val){
            SetHeader(ColorField::Update(val, GetHeader()));
        }

        void SetForwardingAddress(uintptr_t address){
            if(IsForwarding()) return;
            SetForwarding(true);
            forwarding_address_ = address;
        }

        friend class Allocator;
        friend class MarkCopyScavenger;
        friend class Reference;
        friend class WeakReference;
        friend class StrongReference;
        friend class Marker;
    public:
        RawObject(Color color, size_t size, void* ptr):
                owned_(),
                ptr_(ptr),
                pointing_(),
                header_(0),
                forwarding_address_(0){
            SetForwarding(false);
            SetCondemned(false);
            SetColor(color);
            SetSize(size);
        }
        ~RawObject(){
            free(ptr_);
        }

        uintptr_t GetForwardingAddress() const{
            return forwarding_address_;
        }

        uintptr_t GetObjectAddress() const{
            return (uintptr_t)GetObjectPointer();
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

        bool IsGarbage() const{
            return GetColor() == Color::kWhite;
        }

        Color GetColor() const{
            return ColorField::Decode(header_);
        }

        Object* GetObject() const{
            return (Object*)GetObjectPointer();
        }

        bool IsReachable() const;
        std::string ToString() const;
        bool VisitOwnedReferences(ObjectPointerVisitor* vis) const;
        bool VisitPointingReferences(ObjectPointerVisitor* vis) const;

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
}

#endif //TOKEN_RAW_OBJECT_H