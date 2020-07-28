#include <algorithm>
#include "heap.h"

namespace Token{
    void* Semispace::Allocate(size_t alloc_size){
        if((GetAllocatedSize() + alloc_size) > GetTotalSize()) return nullptr;
        void* ptr = (void*)current_;
        if(!ptr) return nullptr;
        current_ += alloc_size;
        memset(ptr, 0, alloc_size);
        return ptr;
    }

    void Semispace::Accept(ObjectPointerVisitor* vis){
        uword current = GetStartAddress();
        while(current < GetCurrentAddress()){
            RawObject* obj = (RawObject*)current;
            if(!vis->Visit(obj)) return;
            current += obj->GetAllocatedSize();
        }
    }

    void Heap::Accept(ObjectPointerVisitor* vis){
        uword current = GetStartAddress();
        while(current < GetEndAddress()){
            RawObject* obj = (RawObject*)current;
            if(!obj || obj->GetAllocatedSize() == 0 || !vis->Visit(obj)) return;
            current += obj->GetAllocatedSize();
        }
    }
}