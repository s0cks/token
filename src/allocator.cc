#include <bitset>
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

    class RootPage{
    public:
        static const size_t kNumberOfRootsPerPage = 1000;
    private:
        RootPage* next_;
        std::bitset<kNumberOfRootsPerPage> test_;
        size_t size_;
        Object* roots_[kNumberOfRootsPerPage];
    public:
        RootPage():
            next_(nullptr),
            test_(),
            size_(0){
            for(size_t idx = 0; idx < kNumberOfRootsPerPage; idx++){
                roots_[idx] = nullptr;
            }
        }
        ~RootPage(){}

        RootPage* GetNext() const{
            return next_;
        }

        Object** Allocate(){
            for(size_t i = 0; i < kNumberOfRootsPerPage; i++){
                if(!test_.test(i)){
                    test_.set(i);
                    size_++;
                    return &roots_[i];
                }
            }
            if(!next_) next_ = new RootPage();
            return next_->Allocate();
        }

        void Write(Object** ptr, Object* data){
            if(data && !data->GetStats()->IsStackSpace()){
                data->IncrementReferenceCount();
            }
            if((*ptr) && !(*ptr)->GetStats()->IsStackSpace()){
                (*ptr)->DecrementReferenceCount();
            }
            (*ptr) = data;
        }

        void Free(Object** ptr){
            ptrdiff_t offset = ptr - roots_;
            if(offset >= 0 && offset < static_cast<int>(kNumberOfRootsPerPage)){
                Write(ptr, nullptr);
                test_.reset(offset);
                size_--;
            } else{
                next_->Free(ptr);
                if(!next_->size_){
                    RootPage* tmp = next_;
                    next_ = tmp->next_;
                    tmp->next_ = nullptr;
                    delete tmp;
                }
            }
        }

        void Accept(RootObjectPointerVisitor* vis){
            if (size_) {
                for (size_t i = 0; i < kNumberOfRootsPerPage; i++){
                    Object* data = roots_[i];
                    if(data && !data->GetStats()->IsStackSpace()){
                        LOG(INFO) << "visiting: " << std::hex << data;
                        vis->Visit(&roots_[i]);
                    }
                }
            }
        }
    };

    static Heap* eden_ = nullptr;
    static Heap* survivor_ = nullptr;
    static Heap* tenured_ = nullptr;
    static RootPage* roots_ = nullptr;

    static size_t allocating_size_ = 0;
    static void* allocating_ = nullptr;

    void Allocator::Initialize(){
        eden_ = new Heap(Space::kEdenSpace, kSemispaceSize);
        survivor_ = new Heap(Space::kSurvivorSpace, kSemispaceSize);
        tenured_ = new Heap(Space::kTenuredSpace, kSemispaceSize);
    }

    Heap* Allocator::GetEdenHeap(){
        return eden_;
    }

    Heap* Allocator::GetSurvivorHeap(){
        return survivor_;
    }

    Object** Allocator::AllocateRoot(Object* obj){
        if(!roots_) roots_ = new RootPage();
        Object** ptr = roots_->Allocate();
        roots_->Write(ptr, obj);
        return ptr;
    }

    void Allocator::FreeRoot(Object** obj){
        if(roots_) roots_->Free(obj);
    }

    void Allocator::VisitRoots(RootObjectPointerVisitor* vis){
        RootPage* current = roots_;
        while(current != nullptr){
            current->Accept(vis);
            current = current->GetNext();
        }
    }

    void* Allocator::Allocate(size_t size){
        allocating_size_ = size;

        void* ptr = GetEdenHeap()->Allocate(size);
        if(!ptr){
            Scavenger::Scavenge(GetEdenHeap());//TODO: convert to MinorGC?
            ptr = GetEdenHeap()->Allocate(size);
            assert(ptr);
        }

        allocating_ = ptr;
        return ptr;
    }

    void Allocator::Initialize(Object* obj){
         if(allocating_ != obj){
            obj->stats_ = GCStats(Space::kStackSpace);
            return;
        }

        obj->stats_ = GCStats(Space::kEdenSpace);
        obj->SetReferenceCount(0);
        obj->SetSize(allocating_size_);
        obj->SetColor(Object::kWhite);
        allocating_size_ = 0;
        allocating_ = nullptr;
    }

    void Allocator::MinorCollect(){
        Scavenger::Scavenge(GetEdenHeap());
        Scavenger::Scavenge(GetSurvivorHeap());
    }

    void Allocator::MajorCollect(){

    }
}