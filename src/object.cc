#include <bitset>
#include "allocator.h"
#include "object.h"
#include "bitfield.h"

namespace Token{
    enum HeaderLayout{
        kColorPosition = 0,
        kBitsForColor = 8,
        kTypePosition = kColorPosition+kBitsForColor,
        kBitsForType = 16,//TODO: convert to class ID
        kSizePosition = kTypePosition+kBitsForType,
        kBitsForSize = 32,
    }; // total size := 56/64 bits

    class ColorField : public BitField<ObjectHeader, Object::Color, kColorPosition, kBitsForColor>{};
    class TypeField : public BitField<ObjectHeader, Object::Type, kTypePosition, kBitsForType>{};
    class SizeField : public BitField<ObjectHeader, uint32_t, kSizePosition, kBitsForSize>{};

    void* Object::operator new(size_t size){
        return Allocator::Allocate(size);
    }

    Object::Object(){
        Allocator::Initialize(this);
    }

    Object::~Object(){
        if(space_ == kStackSpace) Allocator::UntrackStackObject(this);
    }

    void Object::SetColor(Color color){
        SetHeader(ColorField::Update(color, GetHeader()));
    }

    void Object::SetSize(uint32_t size){
        SetHeader(SizeField::Update(size, GetHeader()));
    }

    void Object::SetType(Type type){
        SetHeader(TypeField::Update(type, GetHeader()));
    }

    Object::Color Object::GetColor() const{
        return ColorField::Decode(GetHeader());
    }

    Object::Type Object::GetType() const{
        return TypeField::Decode(GetHeader());
    }

    uint32_t Object::GetSize() const{
        return SizeField::Decode(GetHeader());
    }

    ArrayBase::ArrayBase(size_t length): length_(length){
        for (size_t i = 0; i < length; i++) {
            slots_[i] = nullptr;
        }
    }

    void* ArrayBase::operator new(size_t size, size_t length, bool){
        return Object::operator new(size + sizeof(Object*) * (length - 1));
    }

    void ArrayBase::operator delete(void*, size_t, bool){}

    void ArrayBase::Accept(const FieldIterator& iter) {
        for (size_t i = 0; i < Length(); i++) {
            iter(&slots_[i]);
        }
    }
}