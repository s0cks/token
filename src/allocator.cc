#include <set>
#include <bitset>
#include <cassert>

#include "heap.h"
#include "bitfield.h"
#include "reference.h"
#include "allocator.h"
#include "scavenger.h"

namespace Token{
    static std::recursive_mutex mutex_;
    static std::condition_variable cond_;
    static MemoryRegion* region_ = nullptr;
    static Heap* new_heap_ = nullptr;
    static Heap* old_heap_ = nullptr;
    static std::set<uword> roots_;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static intptr_t allocating_size_ = 0;
    static void* allocating_ = nullptr;

    void Allocator::Initialize(){
        LOCK_GUARD;
        intptr_t region_size = FLAGS_heap_size;
        intptr_t heap_size = (region_size / 4);
        intptr_t semispace_size = (heap_size / 2);
        region_ = new MemoryRegion(region_size);

        uword new_start = region_->GetStartAddress();
        uword old_start = region_->GetStartAddress() + heap_size;

        new_heap_ = new Heap(Space::kNewHeap, new_start, heap_size, semispace_size);
        old_heap_ = new Heap(Space::kOldHeap, old_start, heap_size, semispace_size);
    }

    MemoryRegion* Allocator::GetRegion(){
        LOCK_GUARD;
        return region_;
    }

    Heap* Allocator::GetNewHeap(){
        LOCK_GUARD;
        return new_heap_;
    }

    Heap* Allocator::GetOldHeap(){
        LOCK_GUARD;
        return old_heap_;
    }

#define ALIGN(Size) (((Size)+7)&~7)

    void* Allocator::Allocate(size_t alloc_size){
        LOCK_GUARD;
        size_t total_size = ALIGN(alloc_size);
        void* ptr = GetNewHeap()->Allocate(total_size);
        if(!ptr){
            MinorCollect();
            ptr = GetNewHeap()->Allocate(total_size);
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
        obj->SetSpace(Space::kNewHeap);
        allocating_size_ = 0;
        allocating_ = nullptr;
    }

    void Allocator::MinorCollect(){
        LOG(INFO) << "performing minor garbage collection....";
        LOCK_GUARD;
        Scavenger::Scavenge(false);
    }

    void Allocator::MajorCollect(){

    }

    void Allocator::PrintNewHeap(){
        LOCK_GUARD;
        LOG(INFO) << "New Heap:";
        ObjectPointerPrinter printer;
        if(!Allocator::GetNewHeap()->Accept(&printer))
            LOG(WARNING) << "couldn't print new heap";
    }
}