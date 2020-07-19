#include <algorithm>
#include "allocator.h"
#include "alloc/heap.h"
#include "object.h"

namespace Token{
    static Heap* eden_ = nullptr;
    static Heap* survivor_ = nullptr;
    static Heap* tenured_ = nullptr;
    static Semispace* survivor_from_ = nullptr;
    static Semispace* survivor_to_ = nullptr;

    static Object stack_space_{};
    static size_t allocating_size_ = 0;
    static void* allocating_ = nullptr;

    void Allocator::Initialize(){
        eden_ = new Heap(FLAGS_heap_size);
        survivor_ = new Heap(FLAGS_heap_size);
        tenured_ = new Heap(FLAGS_heap_size);

        uint64_t semi_size = FLAGS_heap_size/2;
        survivor_from_ = new Semispace(survivor_->GetStartAddress(), semi_size);
        survivor_to_ = new Semispace(survivor_->GetStartAddress() + semi_size, semi_size);

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

    Heap* Allocator::GetTenuredHeap(){
        return tenured_;
    }

    Semispace* Allocator::GetSurvivorFromSpace(){
        return survivor_from_;
    }

    Semispace* Allocator::GetSurvivorToSpace(){
        return survivor_to_;
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

    void Allocator::MajorGC(){
        MarkObjects<HeapMemoryIterator>(GetEdenHeap());
        MarkObjects<SemispaceIterator>(GetSurvivorFromSpace());
        MarkObjects<HeapMemoryIterator>(GetTenuredHeap());
        // mark LOS

        FinalizeObjects<HeapMemoryIterator>(GetEdenHeap());
        FinalizeObjects<SemispaceIterator>(GetSurvivorFromSpace());
        FinalizeObjects<HeapMemoryIterator>(GetTenuredHeap());
        // finalize LOS

        tenured_->Clear();

        EvacuateEdenObjects();
        EvacuateSurvivorObjects();
        EvacuateTenuredObjects();

        NotifyWeakReferences<false, HeapMemoryIterator>(GetEdenHeap());
        NotifyWeakReferences<false, SemispaceIterator>(GetSurvivorFromSpace());
        NotifyWeakReferences<false, HeapMemoryIterator>(GetTenuredHeap());
        NotifyWeakReferences<true, StackSpaceIterator>({});

        UpdateStackReferences();
        UpdateNonRootReferences<HeapMemoryIterator>(GetEdenHeap());
        UpdateNonRootReferences<SemispaceIterator>(GetSurvivorFromSpace());
        UpdateNonRootReferences<HeapMemoryIterator>(GetTenuredHeap());
        // update LOS

        CopyLiveObjects<HeapMemoryIterator>(GetEdenHeap());
        CopyLiveObjects<SemispaceIterator>(GetSurvivorFromSpace());
        CopyLiveObjects<HeapMemoryIterator>(GetTenuredHeap());
        // clean LOS

        GetEdenHeap()->Clear();
        GetSurvivorFromSpace()->Clear();

        std::swap(survivor_from_, survivor_to_);
    }

    void Allocator::MinorGC(){
        MarkObjects<HeapMemoryIterator>(GetEdenHeap());
        MarkObjects<SemispaceIterator>(GetSurvivorFromSpace());

        FinalizeObjects<HeapMemoryIterator>(GetEdenHeap());
        FinalizeObjects<SemispaceIterator>(GetSurvivorFromSpace());

        EvacuateEdenObjects();
        EvacuateSurvivorObjects();

        NotifyWeakReferences<false, HeapMemoryIterator>(GetEdenHeap());
        NotifyWeakReferences<false, SemispaceIterator>(GetSurvivorFromSpace());
        NotifyWeakReferences<true, StackSpaceIterator>({});

        UpdateStackReferences();
        UpdateNonRootReferences<HeapMemoryIterator>(GetEdenHeap());
        UpdateNonRootReferences<SemispaceIterator>(GetSurvivorFromSpace());

        CopyLiveObjects<HeapMemoryIterator>(GetEdenHeap());
        CopyLiveObjects<SemispaceIterator>(GetSurvivorFromSpace());

        GetEdenHeap()->Clear();
        GetSurvivorFromSpace()->Clear();

        std::swap(survivor_from_, survivor_to_);
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

    void Allocator::EvacuateEdenObjects(){
        // eden heap -> survivor heap (to space)
        for(Object* obj : Iterable<HeapMemoryIterator>{ GetEdenHeap() }){
            if(!obj->IsGarbage()){
                void* nptr = GetSurvivorToSpace()->Allocate(obj->GetSize());
                LOG(INFO) << "evacuating object: " << std::hex << obj << " to " << nptr;
                obj->ptr_ = static_cast<Object*>(nptr);
            }
        }
    }

    void Allocator::EvacuateSurvivorObjects(){
        // survivor heap (from space) -> survivor heap (to space)
        for(Object* obj : Iterable<SemispaceIterator>{ GetSurvivorFromSpace() }){
            if(!obj->IsGarbage()){
                void* nptr = GetSurvivorToSpace()->Allocate(obj->GetSize());
                LOG(INFO) << "evacuating object: " << std::hex << obj << " to " << nptr;
                obj->ptr_ = static_cast<Object*>(nptr);
            }
        }
    }

    void Allocator::EvacuateTenuredObjects(){
        //TODO: implement EvacuateTenuredObjects()
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

    class DecRefIterator : public FieldIterator{
    public:
        void operator()(Object** field) const{
            Object* obj = (*field);
            if(!obj) return;
            obj->DecrementReferenceCount();
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