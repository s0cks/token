#include <bitset>
#include "handle.h"
#include "object.h"
#include "allocator.h"

namespace Token{
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
                if(data){
                    LOG(INFO) << "visiting (@" << data << "): " << data->ToString();
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

    HandleBase::HandleBase(){
        if(!root_) root_ = new HandleGroup();
        ptr_ = nullptr;
    }

    HandleBase::HandleBase(Object* obj){
        if(!root_) root_ = new HandleGroup();
        ptr_ = root_->Allocate();
        root_->Write(ptr_, obj);
    }

    HandleBase::HandleBase(const HandleBase& h){
        if(h.ptr_){
            ptr_ = root_->Allocate();
            root_->Write(ptr_, (*h.ptr_));
        } else{
            ptr_ = nullptr;
        }
    }

    HandleBase::~HandleBase(){
        if(ptr_){
            root_->Free(ptr_);
        }
    }

    void HandleBase::operator=(Object* obj){
        if(!ptr_)
            ptr_ = root_->Allocate();
        root_->Write(ptr_, obj);
    }

    void HandleBase::operator=(const HandleBase& h){
        if(h.ptr_){
            ptr_ = root_->Allocate();
            root_->Write(ptr_, (*h.ptr_));
        } else{
            ptr_ = nullptr;
        }
    }

    bool HandleBase::VisitHandles(WeakObjectPointerVisitor* vis){
        HandleGroup* group = root_;
        while(group != nullptr){
            if(!group->Accept(vis))
                return false;
            group = group->next_;
        }
        return true;
    }

    bool HandleBase::VisitHandles(ObjectPointerVisitor* vis){
        HandleGroup* group = root_;
        while(group != nullptr){
            if(!group->Accept(vis))
                return false;
            group = group->next_;
        }
        return true;
    }
}