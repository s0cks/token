#include "heap.h"
#include "raw_object.h"

namespace Token{
    RawObject* Semispace::Allocate(uintptr_t alloc_size){
        uintptr_t total_size = alloc_size + sizeof(RawObject);
        if((GetAllocatedSize() + total_size) > GetTotalSize()){
            LOG(WARNING) << "allocated size: " << GetAllocatedSize();
            LOG(WARNING) << "object too large for semispace!: " << total_size << "/" << GetTotalSize();
            return nullptr;
        }
        void* ptr = (void*)current_;
        if(!ptr){
            LOG(WARNING) << "nullptr in semispace region";
            return nullptr;
        }
        current_ += total_size;
        memset(ptr, 0, total_size);
        RawObject* obj = (RawObject*)ptr;
        return nullptr; //TODO: fixme new (obj)RawObject(Color::kWhite, alloc_size);
    }
}