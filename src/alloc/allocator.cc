#include <set>
#include <bitset>
#include <cassert>

#include "heap.h"
#include "bitfield.h"
#include "alloc/allocator.h"
#include "scavenger.h"

namespace Token{
    static std::mutex mutex_;
    static std::condition_variable cond_;
    static MemoryRegion* region_ = nullptr;
    static Heap* new_heap_ = nullptr;
    static Heap* old_heap_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static intptr_t allocating_size_ = 0;
    static void* allocating_ = nullptr;

#define KB_SiZE 1000
#define MB_SIZE 1000000
#define GB_SIZE 1000000000

    static inline intptr_t
    ParseHeapSize(const std::string& size){
        intptr_t scalar = atoll(size.substr(0, size.size()-1).data());
        char last = tolower(size[size.size() - 1]);
        if(last == 'b'){
            return scalar;
        } else if(last == 'm'){
            return scalar * MB_SIZE;
        } else if(last == 'g'){
            return scalar * GB_SIZE;
        } else{
            LOG(ERROR) << "cannot parse heap size: " << size;
            //TODO: Properly shutdown the system?
            return Allocator::kDefaultHeapSize;
        }
    }

    const intptr_t Allocator::kDefaultHeapSize = 8 * MB_SIZE;
    const char* Allocator::kDefaultHeapSizeAsString = "8m";

    void Allocator::Initialize(){
        LOCK_GUARD;

        intptr_t region_size = ParseHeapSize(FLAGS_heap_size);
        intptr_t heap_size = (region_size / 4);
        intptr_t semispace_size = (heap_size / 2);
        region_ = new MemoryRegion(region_size);

        uword new_start = region_->GetStartAddress();
        uword old_start = region_->GetStartAddress() + heap_size;

        new_heap_ = new Heap(Space::kNewHeap, new_start, heap_size, semispace_size);
        old_heap_ = new Heap(Space::kOldHeap, old_start, heap_size, semispace_size);
    }

    MemoryRegion* Allocator::GetRegion(){
        return region_;
    }

    Heap* Allocator::GetNewHeap(){
        return new_heap_;
    }

    Heap* Allocator::GetOldHeap(){
        return old_heap_;
    }

#define ALIGN(Size) (((Size)+7)&~7)

    void* Allocator::AllocateMemory(int64_t size, Space space){
        LOCK_GUARD;
        switch(space){
            case Space::kNewHeap:
                return GetNewHeap()->Allocate(size);
            case Space::kOldHeap:
                return GetOldHeap()->Allocate(size);
            default:
                LOG(ERROR) << "cannot allocate to space: " << space;
                return nullptr;
        }
    }

    void* Allocator::Allocate(int64_t size, Space space){
        int64_t total_size = ALIGN(size);
        void* ptr = AllocateMemory(total_size, space);
        if(!ptr){
            MinorCollect();
            ptr = AllocateMemory(total_size, space);
            assert(ptr);
        }

        allocating_size_ = size;
        allocating_ = ptr;
        return ptr;
    }

    void Allocator::Initialize(Object* obj){
        if(allocating_ != obj){
            return;
        }
        obj->SetSize(allocating_size_);
        allocating_size_ = 0;
        allocating_ = nullptr;
    }

    bool Allocator::MinorCollect(){
        LOCK_GUARD;
        return Scavenger::Scavenge(false);
    }

    bool Allocator::MajorCollect(){
        LOCK_GUARD;
        return Scavenger::Scavenge(true);
    }

    void Allocator::PrintNewHeap(){
        LOG(INFO) << "New Heap:";

        LOCK_GUARD;
        ObjectPointerPrinter printer;
        if(!Allocator::GetNewHeap()->VisitObjects(&printer))
            LOG(WARNING) << "couldn't print new heap";
    }

    void Allocator::PrintOldHeap(){
        LOG(INFO) << "Old Heap:";

        LOCK_GUARD;
        ObjectPointerPrinter printer;
        if(!Allocator::GetOldHeap()->VisitObjects(&printer))
            LOG(WARNING) << "couldn't print old heap";
    }
}