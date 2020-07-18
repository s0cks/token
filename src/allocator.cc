#include "allocator.h"
#include <alloc/heap.h>
#include <object.h>

namespace Token{
    static Heap* eden_ = nullptr;
    static Heap* survivor_ = nullptr;
    static Object stack_space_{};
    static size_t allocating_size_ = 0;
    static void* allocating_ = nullptr;

    void Allocator::Initialize(){
        eden_ = new Heap(FLAGS_minheap_size);
        survivor_ = new Heap(FLAGS_maxheap_size);
        stack_space_.stack_.prev_ = &stack_space_;
        stack_space_.stack_.next_ = &stack_space_;
        stack_space_.space_ = Object::kStackSpace;
    }

    Heap* Allocator::GetEdenHeap(){
        return eden_;
    }

    Heap* Allocator::GetSurvivorHeap(){
        return survivor_;
    }

    class StackSpaceIterator{
    private:
        Object* next_;
    public:
        StackSpaceIterator(): next_(stack_space_.stack_.next_){}

        bool HasNext(){
            return next_ != &stack_space_;
        }

        Object* Next(){
            Object* next = next_;
            next_ = next_->stack_.next_;
            return next;
        }
    };

    void Allocator::MinorGC(){
        MarkObjects<HeapMemoryIterator>(GetEdenHeap());
        FinalizeObjects<HeapMemoryIterator>(GetEdenHeap());

        EvacuateLiveObjects(GetEdenHeap(), GetSurvivorHeap());

        NotifyWeakReferences<false, HeapMemoryIterator>(GetEdenHeap());
        NotifyWeakReferences<true, StackSpaceIterator>({});

        UpdateStackReferences();
        UpdateNonRootReferences<HeapMemoryIterator>(GetEdenHeap());
        CopyLiveObjects<HeapMemoryIterator>(GetEdenHeap());
        GetEdenHeap()->Clear();
    }

    template<typename I>
    void Allocator::MarkObjects(Iterable<I> iter){
        for(Object* obj : iter){
            LOG(INFO) << "checking object: " << std::hex << obj;
            if(obj->HasStackReferences()){
                LOG(INFO) << "marking object: " << std::hex << obj;
                obj->SetColor(Object::kBlack);
            }
        }
    }

    template<typename I>
    void Allocator::FinalizeObjects(Iterable<I> iter){
        for(Object* obj : iter){
            if(obj->IsGarbage()){
                LOG(INFO) << "finalizing object: " << std::hex << obj;
                obj->~Object();
                obj->ptr_ = nullptr;
            }
        }
    }

    void Allocator::EvacuateLiveObjects(Heap* src, Heap* dst){
        for(Object* obj : Iterable<HeapMemoryIterator>{ src }){
            if(!obj->IsGarbage()){
                void* nptr = dst->Allocate(obj->GetSize());
                LOG(INFO) << "evacuating object: " << std::hex << obj << " to " << nptr;
                obj->ptr_ = static_cast<Object*>(nptr);
            }
        }
    }

    template<typename I>
    void Allocator::CopyLiveObjects(Token::Iterable<I> iter){
        for(Object* obj : iter){
            if(!obj->IsGarbage()){
                LOG(INFO) << "copying object: " << std::hex << obj;
                obj->SetColor(Object::kFree);
                memcpy((void*)obj->ptr_, (void*)obj, obj->GetSize());
            }
        }
    }

    class WeakReferenceNotifyIterator : public FieldIterator{
    private:
        Object* target_;
    public:
        WeakReferenceNotifyIterator(Object* target): target_(target){}

        void operator()(Object** field) const{
            Object* obj = (*field);
            if(!obj) return;
            if(!obj->ptr_){
                (*field) = nullptr;
                target_->NotifyWeakReferences(field);
            }
        }
    };

    template<bool asRoot, typename I>
    void Allocator::NotifyWeakReferences(Iterable<I> iter){
        for(Object* obj : iter){
            if(asRoot || !obj->IsGarbage()) obj->Accept(WeakReferenceNotifyIterator{ obj });
        }
    }

    class UpdateIterator : public FieldIterator{
    public:
        void operator()(Object** field) const{
            Object* obj = (*field);
            if(!obj) return;
            (*field) = obj->ptr_;
        }
    };

    template<typename I>
    void Allocator::UpdateNonRootReferences(Iterable<I> iter){
        for(Object* obj : iter){
            if(!obj->IsGarbage()) obj->Accept(UpdateIterator{});
        }
    }

    void Allocator::UpdateStackReferences(){
        for(Object* obj : Iterable<StackSpaceIterator>{}){
            obj->Accept(UpdateIterator{});
        }
    }

    void* Allocator::Allocate(size_t size, Object::Type type){
        allocating_size_ = size;

        void* ptr = GetEdenHeap()->Allocate(size);
        if(!ptr){
            MinorGC();
            ptr = GetEdenHeap()->Allocate(size);
            assert(ptr);
        }

        allocating_ = ptr;
        return ptr;
    }

    void Allocator::Initialize(Object* obj){
        if(allocating_ != obj){
            if(obj != &stack_space_){
                obj->stack_.prev_ = stack_space_.stack_.prev_;
                obj->stack_.next_ = &stack_space_;
                stack_space_.stack_.prev_->stack_.next_ = obj;
                stack_space_.stack_.prev_ = obj;
                obj->space_ = Object::kStackSpace;
            }
            return;
        }

        obj->SetReferenceCount(0);
        obj->SetSpace(Object::kEdenSpace);
        obj->SetSize(allocating_size_);
        obj->SetColor(Object::kWhite);
        allocating_size_ = 0;
        allocating_ = nullptr;
    }
}