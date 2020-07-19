#include <bitset>
#include "handle.h"
#include "object.h"

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
            root_->Write(ptr_, *h.ptr_);
        } else{
            ptr_ = nullptr;
        }
    }

    HandleBase::~HandleBase(){
        if(ptr_) root_->Free(ptr_);
    }

    void HandleBase::operator=(Object* obj){
        if(!ptr_) ptr_ = root_->Allocate();
        root_->Write(ptr_, obj);
    }

    void HandleBase::operator=(const HandleBase& h){
        if(h.ptr_){
            ptr_ = root_->Allocate();
            root_->Write(ptr_, *h.ptr_);
        } else{
            ptr_ = nullptr;
        }
    }
}