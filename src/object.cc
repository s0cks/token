#include <bitset>
#include "allocator.h"
#include "object.h"
#include "bitfield.h"

namespace Token{
    class HandleGroup : public Object{
    private:
        static const size_t kHandlesPerGroup = 984;

        std::bitset<kHandlesPerGroup> bitmap_;
        HandleGroup* next_ = nullptr;
        size_t size_ = 0;
        Object* handles_[kHandlesPerGroup];
    protected:
        void Accept(const FieldIterator& iter){
            if(size_){
                for(size_t i = 0; i < kHandlesPerGroup; i++){
                    Object* data = handles_[i];
                    if(data && data->GetSpace() != Object::kStackSpace){
                        iter(&handles_[i]);
                    }
                }
            }
        }
    public:
        HandleGroup(){
            for(size_t i = 0; i < kHandlesPerGroup; i++){
                handles_[i] = nullptr;
            }
        }

        Object** Allocate(){
            for(size_t i = 0; i < kHandlesPerGroup; i++){
                if(!bitmap_.test(i)){
                    bitmap_.set(i);
                    size_++;
                    return &handles_[i];
                }
            }

            if(!next_) next_ = new HandleGroup();
            return next_->Allocate();
        }

        void Free(Object** ptr){
            ptrdiff_t offset = ptr - handles_;
            if(offset >= 0 && offset < static_cast<int>(kHandlesPerGroup)){
                Write(ptr, nullptr);
                bitmap_.reset(offset);
                size_--;
            } else{
                next_->Free(ptr);
                if(!next_->size_){
                    HandleGroup* tmp = next_;
                    next_ = tmp->next_;
                    tmp->next_ = nullptr;
                    delete tmp;
                }
            }
        }

        void Write(Object** ptr, Object* data){
            if(data && data->GetSpace() != Object::kStackSpace){
                data->IncrementReferenceCount();
            }

            if((*ptr) && (*ptr)->GetSpace() != Object::kStackSpace){
                (*ptr)->DecrementReferenceCount();
            }
            (*ptr) = data;
        }

        void* operator new(size_t size){
            return malloc(size);
        }
    };

    static HandleGroup* root_ = nullptr;

    HandleBase::HandleBase(Object* obj){
        if(!root_) root_ = new HandleGroup();
        ptr_ = root_->Allocate();
        root_->Write(ptr_, obj);
    }

    HandleBase::HandleBase(const HandleBase& h){
        if(h.ptr_){
            ptr_ = root_->Allocate();
            root_->Write(ptr_, *h.ptr_);
        } else{
            ptr_ = nullptr;
        }
    }

    void HandleBase::operator=(Object* obj){
        if(!ptr_) ptr_ = root_->Allocate();
        root_->Write(ptr_, obj);
    }

    void HandleBase::operator=(const HandleBase& h){
        operator=(h.GetPointer());
    }

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

    Object::~Object(){}

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
}