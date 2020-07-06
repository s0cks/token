#if !defined(TOKEN_USE_KOA)
#include "allocator.h"
#include "raw_object.h"
#include "crash_report.h"

namespace Token{
    static uintptr_t allocated_size_ = 0;

    uintptr_t Allocator::GetBytesAllocated(){
        return allocated_size_;
    }

    uintptr_t Allocator::GetBytesFree(){
        return FLAGS_minheap_size - allocated_size_;
    }

    uintptr_t Allocator::GetTotalSize() {
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
        return new (ptr)RawObject(type, size, ptr+sizeof(RawObject));
    }

    bool Allocator::FinalizeObject(RawObject* obj){
        if(IsRoot(obj)){
#ifdef TOKEN_DEBUG
            LOG(WARNING) << "cannot finalize root object: " << (*obj);
#endif//TOKEN_DEBUG
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

        allocated_.erase(obj->GetObjectAddress());
        allocated_size_ -= obj->GetObjectSize();
        free(obj);
        return true;
    }
}

#endif// !TOKEN_USE_KOA