#include <algorithm>
#include "heap.h"

namespace Token{
    void* Semispace::Allocate(size_t alloc_size){
        size_t total_size = alloc_size;
        if((GetAllocatedSize() + total_size) > GetTotalSize()) return nullptr;
        void* ptr = (void*)current_;
        if(!ptr) return nullptr;
        current_ += total_size;
        memset(ptr, 0, total_size);
        return ptr;
    }

    void Semispace::Accept(ObjectPointerVisitor* vis){
        uword current = GetStartAddress();
        while(current < GetCurrentAddress()){
            Object* obj = (Object*)current;
            if(!vis->Visit(obj)) return;
            current += obj->GetAllocatedSize();
        }
    }

    void Heap::Accept(ObjectPointerVisitor* vis){
        uword current = GetStartAddress();
        while(current < GetEndAddress()){
            Object* obj = (Object*)current;
            if(!obj || obj->GetAllocatedSize() == 0 || !vis->Visit(obj)) return;
            current += obj->GetAllocatedSize();
        }
    }
}