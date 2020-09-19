#include "heap.h"

namespace Token{
    void* Semispace::Allocate(intptr_t alloc_size){
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
            Object* obj = (Object*)current;
            if(!vis->Visit(obj)) return;
            current += obj->GetSize();
        }
    }

    bool Heap::VisitObjects(ObjectPointerVisitor* vis){
        std::lock_guard<std::recursive_mutex> guard(mutex_);
        uword current = GetStartAddress();
        while(current < GetEndAddress()){
            Object* obj = (Object*)current;
            if(!obj || obj->GetSize() == 0) break;
            if(!vis->Visit(obj))
                return false;
            current += obj->GetSize();
        }
        return true;
    }

    bool Heap::VisitMarkedObjects(ObjectPointerVisitor* vis){
        std::lock_guard<std::recursive_mutex> guard(mutex_);
        uword current = GetStartAddress();
        while(current < GetEndAddress()){
            Object* obj = (Object*)current;
            if(!obj || obj->GetSize() == 0) break;
            if(obj->IsMarked() && !vis->Visit(obj))
                return false;
            current += obj->GetSize();
        }
        return true;
    }
}