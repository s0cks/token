#include "array.h"
#include "allocator.h"

namespace Token{
    void* ArrayBase::operator new(size_t size, size_t length, bool) {
        return Object::operator new(size + sizeof(Object*) * (length - 1));
    }

    void ArrayBase::operator delete(void*, size_t, bool){}

    ArrayBase::ArrayBase(size_t length): length_(length){
        for (size_t i = 0; i < length; i++) {
            slots_[i] = nullptr;
        }
    }

    bool ArrayBase::Accept(WeakObjectPointerVisitor* vis) {
        for (size_t i = 0; i < Length(); i++){
            if(!vis->Visit(&slots_[i]))
                return false;
        }
        return true;
    }

    std::string ArrayBase::ToString() const{
        std::stringstream ss;
        ss << "Array(" << Length() << ")";
        return ss.str();
    }
}