#ifndef TOKEN_HEAP_H
#define TOKEN_HEAP_H

#include "common.h"
#include "vmemory.h"
#include "object.h"

namespace Token{
    class SemispaceVisitor;
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
        Semispace(const Semispace& other):
            start_(other.start_),
            current_(other.current_),
            size_(other.size_){}
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

        void Reset(){
            memset((void*)start_, 0, size_);
            current_ = start_;
        }

        void* Allocate(size_t size);
        void Accept(ObjectPointerVisitor* vis);

        Semispace& operator=(const Semispace& other){
            start_ = other.start_;
            current_ = other.current_;
            size_ = other.size_;
            return (*this);
        }
    };

    class HeapVisitor;
    class Heap{
    private:
        MemoryRegion region_;
        Space space_;
        Semispace from_space_;
        Semispace to_space_;
    public:
        Heap(Space space, size_t semi_size):
            region_(semi_size*2),
            space_(space),
            from_space_(GetStartAddress(), semi_size),
            to_space_(GetStartAddress() + semi_size, semi_size){}
        ~Heap(){}

        Space GetSpace() const{
            return space_;
        }

        bool IsEdenSpace() const{
            return GetSpace() == Space::kEdenSpace;
        }

        bool IsSurvivorSpace() const{
            return GetSpace() == Space::kSurvivorSpace;
        }

        bool IsTenuredSpace() const{
            return GetSpace() == Space::kTenuredSpace;
        }

        MemoryRegion* GetRegion(){
            return &region_;
        }

        Semispace GetFromSpace(){
            return from_space_;
        }

        Semispace GetToSpace(){
            return to_space_;
        }

        uword GetStartAddress() const{
            return region_.GetStartAddress();
        }

        uword GetEndAddress() const{
            return region_.GetEndAddress();
        }

        bool Contains(uword address) const{
            return address >= GetStartAddress() &&
                   address <= GetEndAddress();
        }

        size_t GetTotalSize() const{
            return region_.GetSize();
        }

        size_t GetAllocatedSize() const{
            return from_space_.GetAllocatedSize();
        }

        size_t GetUnallocatedSize() const{
            return from_space_.GetUnallocatedSize();
        }

        void SwapSpaces(){
            std::swap(from_space_, to_space_);
        }

        void Reset(){
            GetRegion()->Clear();
        }

        void* Allocate(size_t size){
            return from_space_.Allocate(size);
        }

        void Accept(ObjectPointerVisitor* vis);
    };
}

#endif //TOKEN_HEAP_H