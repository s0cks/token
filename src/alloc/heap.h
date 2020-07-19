#ifndef TOKEN_HEAP_H
#define TOKEN_HEAP_H

#include "common.h"
#include "vmemory.h"
#include "object.h"

namespace Token{
    class Heap{
        friend class HeapMemoryIterator;
    private:
        MemoryRegion region_;
        uword start_;
        uword current_;
        size_t size_;

        MemoryRegion* GetRegion(){
            return &region_;
        }
    public:
        Heap(intptr_t size):
            region_(size),
            start_(0),
            current_(0),
            size_(size){
            start_ = current_ = region_.GetStartAddress();
        }
        ~Heap(){}

        uword GetStartAddress() const{
            return region_.GetStartAddress();
        }

        uword GetCurrentAddress() const{
            return current_;
        }

        uword GetEndAddress() const{
            return region_.GetEndAddress();
        }

        size_t GetAllocatedSize() const{
            return GetCurrentAddress() - GetStartAddress();
        }

        size_t GetUnallocatedSize() const{
            return GetEndAddress() - GetCurrentAddress();
        }

        size_t GetTotalSize() const{
            return region_.GetSize();
        }

        void* Allocate(size_t size);

        void Clear(){
            region_.Clear();
            current_ = region_.GetStartAddress();
        }
    };

    class Object;
    class HeapMemoryIterator{
    private:
        MemoryRegion* region_;
        uword current_;
        uword end_;
    public:
        HeapMemoryIterator(Heap* heap):
            region_(heap->GetRegion()),
            current_(heap->GetStartAddress()),
            end_(heap->GetCurrentAddress()){}
        ~HeapMemoryIterator() = default;

        bool HasNext(){
            return current_ < end_;
        }

        Object* Next(){
            Object* next = (Object*)current_;
            current_ += next->GetAllocatedSize();
            return next;
        }
    };
}

#endif //TOKEN_HEAP_H