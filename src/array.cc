#include "array.h"

namespace Token{
    ArrayBase::ArrayBase(size_t length): length_(length){
        for (size_t i = 0; i < length; i++) {
            slots_[i] = nullptr;
        }
    }

    void ArrayBase::Accept(WeakReferenceVisitor* vis) {
        for (size_t i = 0; i < Length(); i++) {
            vis->Visit(&slots_[i]);
        }
    }

    std::string ArrayBase::ToString() const{
        std::stringstream ss;
        ss << "Array(" << Length() << ")";
        return ss.str();
    }
}