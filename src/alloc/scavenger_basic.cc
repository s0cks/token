#if !defined(TOKEN_USE_KOA)
#include "alloc/scavenger.h"
#include "allocator.h"

namespace Token{
    bool Scavenger::ScavengeMemory(){
#ifdef TOKEN_DEBUG
        LOG(INFO) << "performing garbage collection...";
        uintptr_t initial_size = Allocator::GetBytesAllocated();
#endif//TOKEN_DEBUG
        DarkenRoots();
        MarkObjects();

        uintptr_t allocated_size = Allocator::GetBytesAllocated();
        ObjectAddressMap allocated = Allocator::allocated_;
        ObjectAddressMap::iterator iter = allocated.begin();
        while(iter != allocated.end()){
            RawObject* obj = iter->second;
            if(obj->IsGarbage()){
                Allocator::Finalize(obj);
                iter = allocated.erase(iter);
                allocated_size -= obj->GetObjectSize();
            }
            iter++;
        }

        Allocator::allocated_ = allocated;
#ifdef TOKEN_DEBUG
        uintptr_t total_mem_scavenged = (initial_size - Allocator::GetBytesAllocated());
        LOG(INFO) << "garbage collection complete";
        LOG(INFO) << "collected: " << total_mem_scavenged << "/" << Allocator::GetTotalSize() << " bytes";
        LOG(INFO) << "allocated: " << Allocator::GetBytesAllocated() << "/" << Allocator::GetTotalSize() << " bytes";
#endif//TOKEN_DEBUG
        return true;
    }
}

#endif//TOKEN_USE_KOA