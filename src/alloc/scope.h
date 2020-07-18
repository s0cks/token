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
            //TODO: for(auto& ptr : pointers_) Allocator::RemoveRoot((void*)ptr);
        }

        void Retain(Object* obj){
            //Allocator::AddRoot(obj);
            pointers_.push_back(reinterpret_cast<uintptr_t>(obj));
        }

        template<typename Type>
        Type* Retain(Type* ptr){
            //Allocator::AddRoot(ptr);
            pointers_.push_back(reinterpret_cast<uintptr_t>(ptr));
            return ptr;
        }
    };
}

#endif //TOKEN_SCOPE_H