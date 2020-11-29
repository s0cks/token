#include <bitset>
#include "handle.h"
#include "object.h"
#include "allocator.h"

namespace Token{
#ifndef TOKEN_GCMODE_NONE
    class HandleGroup : public Object{
        friend class HandleBase;
    private:
        static const size_t kHandlesPerGroup = 1000;
        std::bitset<kHandlesPerGroup> bitmap_;
        HandleGroup* next_ = nullptr;
        size_t size_ = 0;
        Object* handles_[kHandlesPerGroup];
    protected:
        bool Accept(WeakObjectPointerVisitor* vis){
            for(size_t idx = 0;
                idx < kHandlesPerGroup;
                idx++){
                Object* data = handles_[idx];
                if(data && !data->IsMarked()){
                    if(!vis->Visit(&handles_[idx]))
                        return false;
                }
            }
            return true;
        }

        bool Accept(ObjectPointerVisitor* vis){
            for(size_t idx = 0;
                idx < kHandlesPerGroup;
                idx++){
                Object* data = handles_[idx];
                if(data){
                    if(!vis->Visit(data))
                        return false;
                }
            }
            return true;
        }
    public:
        HandleGroup(){
            for(size_t idx = 0;
                idx < kHandlesPerGroup;
                idx++){
                handles_[idx] = nullptr;
            }
        }

        Object** Allocate(){
            for(size_t idx = 0;
                idx < kHandlesPerGroup;
                idx++){
                if(!bitmap_.test(idx)){
                    bitmap_.set(idx);
                    size_++;
                    return &handles_[idx];
                }
            }

            if(!next_) next_ = new HandleGroup();
            return next_->Allocate();
        }

        void Free(Object** ptr){
            ptrdiff_t offset = ptr - handles_;
            if(offset >= 0 && offset < (ptrdiff_t)kHandlesPerGroup){
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
            if(data){
                data->IncrementReferenceCount();
            }

            if((*ptr)){
                (*ptr)->DecrementReferenceCount();
            }

            (*ptr) = data;
        }

        void* operator new(size_t size){
            return malloc(size);
        }

        void operator delete(void* ptr){
            free(ptr);
        }
    };

    static HandleGroup* root_ = nullptr;

#endif//TOKEN_GCMODE_NONE

    HandleBase::HandleBase(){
#ifndef TOKEN_GCMODE_NONE
        if(!root_) root_ = new HandleGroup();
#endif//TOKEN_GCMODE_NONE
        ptr_ = nullptr;
    }

    HandleBase::HandleBase(Object* obj){
#ifdef TOKEN_GCMODE_NONE
        ptr_ = obj;
#else
        if(!root_) root_ = new HandleGroup();
        ptr_ = root_->Allocate();
        root_->Write(ptr_, obj);
#endif//TOKEN_GCMODE_NONE
    }

    HandleBase::HandleBase(const HandleBase& h){
        if(h.ptr_){
#ifdef TOKEN_GCMODE_NONE
            ptr_ = h.ptr_;
#else
            ptr_ = root_->Allocate();
            root_->Write(ptr_, (*h.ptr_));
#endif//TOKEN_GCMODE_NONE
        } else{
            ptr_ = nullptr;
        }
    }

    HandleBase::~HandleBase(){
        if(ptr_){
#ifdef TOKEN_GCMODE_NONE
            delete ptr_;
#else
            root_->Free(ptr_);
#endif//TOKEN_GCMODE_NONE
        }
    }

    void HandleBase::operator=(Object* obj){
#ifdef TOKEN_GCMODE_NONE
        ptr_ = obj;
#else
        if(!ptr_)
            ptr_ = root_->Allocate();
        root_->Write(ptr_, obj);
#endif//TOKEN_GCMODE_NONE
    }

    void HandleBase::operator=(const HandleBase& h){
#ifdef TOKEN_GCMODE_NONE
        ptr_ = h.ptr_ ? h.ptr_ : nullptr;
#else
        if(h.ptr_){
            ptr_ = root_->Allocate();
            root_->Write(ptr_, (*h.ptr_));
        } else{
            ptr_ = nullptr;
        }
#endif//TOKEN_GCMODE_NONE
    }

#ifndef TOKEN_GCMODE_NONE
    bool HandleBase::VisitHandles(WeakObjectPointerVisitor* vis){
        HandleGroup* group = root_;
        while(group != nullptr){
            if(!group->Accept(vis))
                return false;
            group = group->next_;
        }
        return true;
    }
#endif//TOKEN_GCMODE_NONE
}