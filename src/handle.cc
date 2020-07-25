#include "handle.h"
#include "object.h"

namespace Token{
    HandleBase::HandleBase(){
        ptr_ = nullptr;
    }

    HandleBase::HandleBase(Object* obj){
        ptr_ = Allocator::AllocateRoot(obj);
    }

    HandleBase::HandleBase(const HandleBase& h){
        if(h.ptr_){
            ptr_ = Allocator::AllocateRoot(*h.ptr_);
        } else{
            ptr_ = nullptr;
        }
    }

    HandleBase::~HandleBase(){
        if(ptr_) Allocator::FreeRoot(ptr_);
    }

    void HandleBase::operator=(Object* obj){
        if(!ptr_) ptr_ = Allocator::AllocateRoot(obj);
    }

    void HandleBase::operator=(const HandleBase& h){
        if(h.ptr_){
            ptr_ = Allocator::AllocateRoot(*h.ptr_);
        } else{
            ptr_ = nullptr;
        }
    }
}