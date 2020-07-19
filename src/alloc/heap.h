#ifndef TOKEN_HEAP_H
#define TOKEN_HEAP_H

#include "common.h"
#include "vmemory.h"
#include "object.h"

namespace Token{
    class Semispace{
        friend class Heap;
    private:
        uword start_;
        uword current_;
        size_t size_;
    public:
        Semispace():
            start_(0),
            current_(0),
            size_(0){}
        Semispace(uword start, size_t size):
            start_(start),
            current_(start),
            size_(size){}
        ~Semispace() = default;

        uword GetStartAddress() const{
            return start_;
        }

        uword GetCurrentAddress() const{
            return current_;
        }

        uword GetEndAddress() const{
            return GetStartAddress() + GetTotalSize();
        }

        size_t GetAllocatedSize() const{
            return GetCurrentAddress() - GetStartAddress();
        }

        size_t GetUnallocatedSize() const{
            return GetEndAddress() - GetCurrentAddress();
        }

        size_t GetTotalSize() const{
            return size_;
        }

        bool Contains(uword address) const{
            return address >= GetStartAddress() &&
                    address <= GetEndAddress();
        }

        void Clear(){
            memset((void*)start_, 0, size_);
            current_ = start_;
        }

        void* Allocate(size_t size);
    };

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

        bool Contains(uword address) const{
            return address >= GetStartAddress() &&
                    address <= GetEndAddress();
        }

        void Clear(){
            region_.Clear();
            current_ = region_.GetStartAddress();
        }

        void* Allocate(size_t size);
    };

    class Object;
    class HeapMemoryIterator{
    private:
        uword current_;
        uword end_;
    public:
        HeapMemoryIterator(Heap* heap):
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

    class SemispaceIterator{
    private:
        uword current_;
        uword end_;
    public:
        SemispaceIterator(Semispace* semispace):
            current_(semispace->GetStartAddress()),
            end_(semispace->GetEndAddress()){}
        ~SemispaceIterator() = default;

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