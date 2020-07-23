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

    void* SinglespaceHeap::Allocate(size_t alloc_size){
        size_t total_size = alloc_size;
        if((GetAllocatedSize() + total_size) > GetTotalSize()) return nullptr;
        void* ptr = (void*)current_;
        if(!ptr) return nullptr;
        current_ += total_size;
        memset(ptr, 0, total_size);
        return ptr;
    }
}