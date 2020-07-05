#ifndef TOKEN_SCOPE_H
#define TOKEN_SCOPE_H

#include "common.h"
#include "allocator.h"
#include "object.h"

namespace Token{
    class Scope{
    private:
        std::vector<uintptr_t> pointers_;
    public:
        Scope():
            pointers_(){}
        ~Scope(){
            for(auto& ptr : pointers_) Allocator::RemoveRoot((void*)ptr);
        }

        void Retain(Object* obj){
            Allocator::AddRoot(obj);
            pointers_.push_back(reinterpret_cast<uintptr_t>(obj));
        }
    };
}

#endif //TOKEN_SCOPE_H