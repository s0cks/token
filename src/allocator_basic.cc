#if !defined(TOKEN_USE_KOA)
#include "allocator.h"
#include "alloc/raw_object.h"
#include "crash_report.h"

namespace Token{
    static uintptr_t allocated_size_ = 0;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    uintptr_t Allocator::GetBytesAllocated(){
        LOCK_GUARD;
        return allocated_size_;
    }

    uintptr_t Allocator::GetBytesFree(){
        LOCK_GUARD;
        return FLAGS_minheap_size - allocated_size_;
    }

    uintptr_t Allocator::GetTotalSize() {
        LOCK_GUARD;
        return FLAGS_minheap_size;
    }

    RawObject* Allocator::AllocateObject(uintptr_t size, Object::Type type){
        void* ptr = nullptr;
        if(!(ptr = malloc(size))){
            // should never happen?
            std::stringstream ss;
            ss << "Couldn't allocate object of size: " << size;
            CrashReport::GenerateAndExit(ss);
        }
        memset(ptr, 0, size);
        return new (ptr)RawObject(type, size);
    }

    bool Allocator::FinalizeObject(RawObject* obj){
        LOCK_GUARD;
        if(IsRoot(obj)){
            LOG(WARNING) << "cannot finalize root object: " << (*obj);
            return false;
        }

#ifdef TOKEN_DEBUG
        LOG(INFO) << "finalizing object: " << (*obj);
#endif//TOKEN_DEBUG
        Object* o = (Object*)obj->GetObjectPointer();
        if(!o->Finalize()){
            std::stringstream ss;
            ss << "Couldn't finalize object: " << (*obj);
            CrashReport::GenerateAndExit(ss);
        }

        allocated_size_ -= obj->GetObjectSize();
        free(obj);
        return true;
    }
}

#endif// !TOKEN_USE_KOA