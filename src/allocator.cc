#include <cassert>
#include <bitset>
#include <algorithm>

#include "allocator.h"
#include "heap.h"
#include "bitfield.h"
#include "scavenger.h"

namespace Token{
    const size_t Allocator::kNumberOfHeaps = 3;
    const size_t Allocator::kTotalSize = FLAGS_heap_size;
    const size_t Allocator::kHeapSize = kTotalSize/kNumberOfHeaps;
    const size_t Allocator::kSemispaceSize = kHeapSize/2;

    class RootPage{
    public:
        static const size_t kNumberOfRootsPerPage = 1000;
    private:
        RootPage* next_;
        std::bitset<kNumberOfRootsPerPage> test_;
        RawObject* roots_[kNumberOfRootsPerPage];
    public:
        RootPage():
            next_(nullptr),
            test_(){
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
            } else{
                if(next_){
                    next_->Free(ptr);
                    if(next_){
                        RootPage* tmp = next_;
                        next_ = tmp->next_;
                        tmp->next_ = nullptr;
                        delete tmp;
                    }
                }
            }
        }

        bool Accept(WeakObjectPointerVisitor* vis){
            for (size_t i = 0; i < kNumberOfRootsPerPage; i++){
                RawObject* data = roots_[i];
                if(data && data->IsInStackSpace()){
                    vis->Visit(&roots_[i]);
                }
            }
            return true; //TODO: investigate proper implementation
        }
    };

    static std::recursive_mutex mutex_;
    static std::condition_variable cond_;
    static Heap* eden_ = nullptr;
    static Heap* survivor_ = nullptr;
    static RootPage* roots_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static size_t allocating_size_ = 0;
    static void* allocating_ = nullptr;

    void Allocator::Initialize(){
        LOCK_GUARD;
        eden_ = new Heap(Space::kEdenSpace, kSemispaceSize);
        survivor_ = new Heap(Space::kSurvivorSpace, kSemispaceSize);
    }

    Heap* Allocator::GetEdenHeap(){
        LOCK_GUARD;
        return eden_;
    }

    Heap* Allocator::GetSurvivorHeap(){
        LOCK_GUARD;
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

    bool Allocator::VisitRoots(WeakObjectPointerVisitor* vis){
        LOCK_GUARD;
        RootPage* current = roots_;
        while(current != nullptr){
            if(!current->Accept(vis)) return false;
            current = current->GetNext();
        }
        return true;
    }

#define ALIGN(Size) (((Size)+7)&~7)

    void* Allocator::Allocate(size_t alloc_size){
        LOCK_GUARD;
        size_t total_size = ALIGN(alloc_size);
        void* ptr = GetEdenHeap()->Allocate(total_size);
        if(!ptr){
            MinorCollect();
            ptr = GetEdenHeap()->Allocate(total_size);
            assert(ptr);
        }
        allocating_size_ = alloc_size;
        allocating_ = ptr;
        return ptr;
    }

    void Allocator::Initialize(RawObject* obj){
        if(allocating_ != obj){
            LOG(INFO) << "initializing stack object: " << std::hex << obj;
            obj->SetSpace(Space::kStackSpace);
            return;
        }
        obj->SetObjectSize(allocating_size_);
        obj->SetSpace(Space::kEdenSpace);
        allocating_size_ = 0;
        allocating_ = nullptr;
    }

    void Allocator::MinorCollect(){
        LOG(INFO) << "performing minor garbage collection....";
        LOCK_GUARD;
        Scavenger::Scavenge(GetEdenHeap());
        Scavenger::Scavenge(GetSurvivorHeap());
    }

    void Allocator::MajorCollect(){

    }

    class StackSpaceSizeCalculator : public WeakObjectPointerVisitor{
    private:
        size_t size_;
        size_t num_objects_;
    public:
        StackSpaceSizeCalculator():
            WeakObjectPointerVisitor(),
            size_(0),
            num_objects_(0){}
        ~StackSpaceSizeCalculator() = default;

        size_t GetSize() const{
            return size_;
        }

        size_t GetNumberOfObjects() const{
            return num_objects_;
        }

        bool Visit(RawObject** root){
            LOG(INFO) << "visiting root: " << (*root);
            size_ += (*root)->GetAllocatedSize();
            num_objects_++;
            return true;
        }
    };

    size_t Allocator::GetStackSpaceSize(){
        LOCK_GUARD;
        StackSpaceSizeCalculator calc;
        if(!VisitRoots(&calc)) return 0;
        return calc.GetSize();
    }

    size_t Allocator::GetNumberOfStackSpaceObjects(){
        LOCK_GUARD;
        StackSpaceSizeCalculator calc;
        if(!VisitRoots(&calc)) return 0;
        return calc.GetNumberOfObjects();
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