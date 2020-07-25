#include <bitset>
#include "allocator.h"
#include "object.h"
#include "bitfield.h"

namespace Token{
    enum HeaderLayout{
        kColorPosition = 0,
        kBitsForColor = 8,
        kSizePosition = kColorPosition+kBitsForColor,
        kBitsForSize = 32,
    }; // total size := 40/64 bits

    class ColorField : public BitField<ObjectHeader, Object::Color, kColorPosition, kBitsForColor>{};
    class SizeField : public BitField<ObjectHeader, uint32_t, kSizePosition, kBitsForSize>{};

    void* Object::operator new(size_t size){
        return Allocator::Allocate(size);
    }

    Object::Object(){
        Allocator::Initialize(this);
    }

    Object::~Object(){
        if(GetStats()->IsStackSpace()) Allocator::UntrackStackObject(this);
    }

    void Object::SetColor(Color color){
        SetHeader(ColorField::Update(color, GetHeader()));
    }

    void Object::SetSize(uint32_t size){
        SetHeader(SizeField::Update(size, GetHeader()));
    }

    Object::Color Object::GetColor() const{
        return ColorField::Decode(GetHeader());
    }

    uint32_t Object::GetSize() const{
        return SizeField::Decode(GetHeader());
    }
}