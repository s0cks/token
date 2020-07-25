#include <algorithm>
#include "allocator.h"
#include "heap.h"
#include "object.h"
#include "scavenger.h"

namespace Token{
    static const size_t kNumberOfHeaps = 3;
    static const size_t kTotalSize = FLAGS_heap_size;
    static const size_t kHeapSize = kTotalSize/kNumberOfHeaps;
    static const size_t kSemispaceSize = kHeapSize/2;

    static Heap* eden_ = nullptr;
    static Heap* survivor_ = nullptr;
    static Heap* tenured_ = nullptr;

    static Object stack_space_{};
    static size_t allocating_size_ = 0;
    static void* allocating_ = nullptr;

    void Allocator::Initialize(){
        eden_ = new Heap(kSemispaceSize);
        survivor_ = new Heap(kSemispaceSize);
        tenured_ = new Heap(kSemispaceSize);

        stack_space_.stack_.prev_ = &stack_space_;
        stack_space_.stack_.next_ = &stack_space_;
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

    class UpdateIterator : public FieldIterator{
    public:
        void operator()(Object** field) const{
            Object* obj = (*field);
            if(!obj) return;
            (*field) = obj->ptr_;
        }
    };

    void Allocator::UpdateStackReferences(){
        for(Object* obj : Iterable<StackSpaceIterator>{}){
            obj->Accept(UpdateIterator{});
        }
    }

    class WeakReferenceNotifier : public FieldIterator{
    private:
        Object* target_;
    public:
        WeakReferenceNotifier(Object* target): target_(target){}

        void operator()(Object** field) const{
            Object* obj = (*field);
            if(!obj) return;
            if(!obj->ptr_){
                (*field) = nullptr;
                target_->NotifyWeakReferences(field);
            }
        }
    };

    void Allocator::NotifyWeakStackReferences(){
        for(Object* obj : Iterable<StackSpaceIterator>{}){
            obj->Accept(WeakReferenceNotifier{ obj });
        }
    }

    class DecRefIterator : public FieldIterator{
    public:
        void operator()(Object** field) const{
            Object* obj = (*field);
            if(!obj) return;
            obj->DecrementReferenceCount();
        }
    };

    void* Allocator::Allocate(size_t size){
        allocating_size_ = size;

        void* ptr = GetEdenHeap()->Allocate(size);
        if(!ptr){
            Scavenger::Scavenge();
            ptr = GetEdenHeap()->Allocate(size);
            assert(ptr);
        }

        allocating_ = ptr;
        return ptr;
    }

    void Allocator::UntrackStackObject(Token::Object* obj){
        obj->Accept(DecRefIterator{});
        obj->stack_.prev_->stack_.next_ = obj->stack_.next_;
        obj->stack_.next_->stack_.prev_ = obj->stack_.prev_;
    }

    void Allocator::Initialize(Object* obj){
        if(allocating_ != obj){
            if(obj != &stack_space_){
                obj->stack_.prev_ = stack_space_.stack_.prev_;
                obj->stack_.next_ = &stack_space_;
                stack_space_.stack_.prev_->stack_.next_ = obj;
                stack_space_.stack_.prev_ = obj;
                obj->stats_ = GCStats(Space::kStackSpace);
            }
            return;
        }

        obj->stats_ = GCStats(Space::kEdenSpace);
        obj->SetReferenceCount(0);
        obj->SetSize(allocating_size_);
        obj->SetColor(Object::kWhite);
        allocating_size_ = 0;
        allocating_ = nullptr;
    }
}