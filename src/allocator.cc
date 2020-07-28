#include <bitset>
#include <algorithm>
#include "allocator.h"
#include "heap.h"
#include "bitfield.h"
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
        RawObject* roots_[kNumberOfRootsPerPage];
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

        RawObject** Allocate(){
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

        void Write(RawObject** ptr, RawObject* data){
            if(data && !data->IsInStackSpace()){
                data->IncrementReferenceCount();
            }
            if((*ptr) && !(*ptr)->IsInStackSpace()){
                (*ptr)->DecrementReferenceCount();
            }
            (*ptr) = data;
        }

        void Free(RawObject** ptr){
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
                    RawObject* data = roots_[i];
                    if(data && !data->IsInStackSpace()){
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

    RawObject** Allocator::TrackRoot(RawObject* obj){
        if(!roots_) roots_ = new RootPage();
        RawObject** ptr = roots_->Allocate();
        roots_->Write(ptr, obj);
        return ptr;
    }

    void Allocator::FreeRoot(RawObject** obj){
        if(roots_) roots_->Free(obj);
    }

    void Allocator::UntrackRoot(RawObject* root){
        //TODO: implement
    }

    void Allocator::VisitRoots(RootObjectPointerVisitor* vis){
        RootPage* current = roots_;
        while(current != nullptr){
            current->Accept(vis);
            current = current->GetNext();
        }
    }

#define ALIGN(Size) (((Size)+7)&~7)

    void* Allocator::Allocate(size_t alloc_size){
        size_t total_size = ALIGN(alloc_size);
        void* ptr = GetEdenHeap()->Allocate(total_size);
        if(!ptr){
            Scavenger::Scavenge(GetEdenHeap());//TODO: convert to MinorGC?
            ptr = GetEdenHeap()->Allocate(total_size);
            assert(ptr);
        }
        allocating_size_ = alloc_size;
        allocating_ = ptr;
        return ptr;
    }

    void Allocator::Initialize(RawObject* obj){
        if(allocating_ != obj){
            obj->SetSpace(Space::kStackSpace);
            return;
        }

        obj->SetObjectSize(allocating_size_);
        obj->SetSpace(Space::kEdenSpace);
        allocating_size_ = 0;
        allocating_ = nullptr;
    }

    void Allocator::MinorCollect(){
        Scavenger::Scavenge(GetEdenHeap());
        Scavenger::Scavenge(GetSurvivorHeap());
    }

    void Allocator::MajorCollect(){

    }

    void RawObject::SetSpace(Space space){
        stats_.space_ = space;
    }

    void RawObject::SetColor(Color color){
        stats_.color_ = color;
    }

    Color RawObject::GetColor() const{
        return stats_.color_;
    }

    void RawObject::SetObjectSize(size_t size){
        stats_.object_size_ = size;
    }

    size_t RawObject::GetObjectSize() const{
        return stats_.object_size_;
    }

    void RawObject::IncrementReferenceCount(){
        stats_.num_references_++;
    }

    void RawObject::DecrementReferenceCount(){
        stats_.num_references_--;
    }

    size_t RawObject::GetReferenceCount() const{
        return stats_.num_references_;
    }
}