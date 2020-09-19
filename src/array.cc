#include "array.h"
#include "allocator.h"

namespace Token{
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