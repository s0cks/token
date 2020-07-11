#if defined(TOKEN_USE_KOA)
#include "allocator.h"
#include "alloc/heap.h"
#include "alloc/raw_object.h"
#include "crash_report.h"

namespace Token{
#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    uintptr_t Allocator::GetBytesAllocated(){
        return GetEdenHeap()->GetFromSpace()->GetAllocatedSize();
    }

    uintptr_t Allocator::GetBytesFree(){
        return GetEdenHeap()->GetFromSpace()->GetUnallocatedSize();
    }

    uintptr_t Allocator::GetTotalSize(){
        return GetEdenHeap()->GetFromSpace()->GetTotalSize();
    }

    RawObject* Allocator::AllocateObject(uintptr_t size, Object::Type type){
        void* ptr = nullptr;
        if(!(ptr = GetEdenHeap()->Allocate(size))){
            std::stringstream ss;
            ss << "Couldn't allocate object of size: " << size;
            CrashReport::GenerateAndExit(ss);
        }
        memset(ptr, 0, size);
        return new (ptr)RawObject(type, size);
    }

    bool Allocator::FinalizeObject(Token::RawObject* obj){
        //TODO: implement me
        return true;
    }
}

#endif//TOKEN_USE_KOA