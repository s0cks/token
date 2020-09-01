#include "handle.h"
#include "object.h"

namespace Token{
    HandleBase::HandleBase(){
        ptr_ = nullptr;
    }

    HandleBase::HandleBase(RawObject* obj){
        ptr_ = Allocator::TrackRoot(obj);
    }

    HandleBase::HandleBase(const HandleBase& h){
        if(h.ptr_){
            ptr_ = Allocator::TrackRoot(*h.ptr_);
        } else{
            ptr_ = nullptr;
        }
    }

    HandleBase::~HandleBase(){
        if(ptr_) Allocator::FreeRoot(ptr_);
    }

    void HandleBase::operator=(RawObject* obj){
        ptr_ = Allocator::TrackRoot(obj);
    }

    void HandleBase::operator=(const HandleBase& h){
        if(h.ptr_){
            ptr_ = Allocator::TrackRoot(h.GetPointer());
        } else{
            ptr_ = nullptr;
        }
    }
}