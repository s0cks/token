#ifndef TOKEN_HEAP_H
#define TOKEN_HEAP_H

#include "common.h"
#include "vmemory.h"

namespace Token{
    class RawObject;

    class Semispace{
    private:
        void* start_;
        uintptr_t size_;
        uintptr_t current_;

        friend class Heap;
    public:
        Semispace():
            start_(nullptr),
            current_(0),
            size_(0){}
        Semispace(void* start, uintptr_t size):
            start_(start),
            current_((uintptr_t)start),
            size_(size){}
        ~Semispace(){}

        uintptr_t GetCurrentAddress() const{
            return current_;
        }

        uintptr_t GetStartAddress() const{
            return (uintptr_t)start_;
        }

        uintptr_t GetEndAddress() const{
            return (uintptr_t)(start_ + size_);
        }

        uintptr_t GetAllocatedSize() const{
            return GetCurrentAddress() - GetStartAddress();
        }

        uintptr_t GetUnallocatedSize() const{
            return GetEndAddress() - GetCurrentAddress();
        }

        uintptr_t GetTotalSize() const{
            return size_;
        }

        bool Contains(uintptr_t address){
            return (address >= GetStartAddress()) ||
                    (address <= GetEndAddress());
        }

        bool Clear(){
            memset(start_, 0, size_);
            current_ = (uintptr_t)start_;
            return GetAllocatedSize() == 0;
        }

        void* Allocate(uintptr_t size);

        Semispace& operator=(const Semispace& other){
            start_ = other.start_;
            current_ = other.current_;
            size_ = other.size_;
            return (*this);
        }
    };

    class Heap{
    private:
        MemoryRegion region_;
        Semispace from_space_;
        Semispace to_space_;
        uintptr_t heap_size_;
        uintptr_t semi_size_;
    public:
        Heap(uintptr_t size):
            region_(size),
            from_space_(),
            to_space_(),
            heap_size_(0),
            semi_size_(0){
#if defined(TOKEN_VMEMORY_USE_MEMORYMAP)
            if(!region_.Protect(MemoryRegion::kReadWrite)){
                LOG(WARNING) << "couldn't set read/write protections on heap memory region";
                return;
            }
#endif
            heap_size_ = region_.GetSize();
            semi_size_ = (region_.GetSize() / 2);

            LOG(INFO) << "Heap Size := " << heap_size_;
            LOG(INFO) << "Semispace Size := " << semi_size_;

            from_space_ = Semispace(region_.GetPointer(), semi_size_);
            to_space_ = Semispace(region_.GetPointer() + semi_size_, semi_size_);
        }
        ~Heap() = default;

        uintptr_t GetSemispaceSize() const{
            return semi_size_;
        }

        uintptr_t GetTotalSize() const{
            return heap_size_;
        }

        uintptr_t GetStartAddress() const{
            return region_.GetStart();
        }

        uintptr_t GetEndAddress() const{
            return region_.GetEnd();
        }

        Semispace* GetFromSpace(){
            return &from_space_;
        }

        Semispace* GetToSpace(){
            return &to_space_;
        }

        void SwapSpaces(){
            Semispace tmp = from_space_;
            from_space_ = to_space_;
            to_space_ = tmp;
            to_space_.Clear();
        }

        bool Contains(uintptr_t address) const{
            return (address >= GetStartAddress()) ||
                    (address <= GetEndAddress());
        }

        void* Allocate(uintptr_t size){
            return GetFromSpace()->Allocate(size);
        }
    };
}

#endif //TOKEN_HEAP_H