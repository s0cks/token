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

        void Reset(){
            memset((void*)start_, 0, size_);
            current_ = start_;
        }

        void* Allocate(size_t size);

        Semispace& operator=(const Semispace& other){
            start_ = other.start_;
            current_ = other.current_;
            size_ = other.size_;
            return (*this);
        }
    };

    class Heap{
        friend class HeapMemoryIterator;
    private:
        MemoryRegion region_;
    protected:
        Heap(size_t size):
            region_(size){}
    public:
        virtual ~Heap() = default;

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

        virtual void* Allocate(size_t size) = 0;
        virtual size_t GetAllocatedSize() const = 0;
        virtual size_t GetUnallocatedSize() const = 0;

        virtual void Reset(){
            region_.Clear();
        }
    };

    class SinglespaceHeap : public Heap{
    private:
        uword current_;
    public:
        SinglespaceHeap(size_t size):
            Heap(size),
            current_(GetStartAddress()){}
        ~SinglespaceHeap(){}

        uword GetCurrentAddress() const{
            return current_;
        }

        size_t GetAllocatedSize() const{
            return GetCurrentAddress() - GetStartAddress();
        }

        size_t GetUnallocatedSize() const{
            return GetEndAddress() - GetCurrentAddress();
        }

        void* Allocate(size_t size);
    };

    class SemispaceHeap : public Heap{
        friend class HeapMemoryIterator;
    private:
        Semispace from_;
        Semispace to_;
    public:
        SemispaceHeap(size_t size):
            Heap(size),
            from_(GetStartAddress(), size/2),
            to_(GetStartAddress()+(size/2), size/2){}
        ~SemispaceHeap(){}

        Semispace* GetFromSpace(){
            return &from_;
        }

        Semispace* GetToSpace(){
            return &to_;
        }

        size_t GetAllocatedSize() const{
            return from_.GetAllocatedSize();
        }

        size_t GetUnallocatedSize() const{
            return from_.GetUnallocatedSize();
        }

        void* Allocate(size_t size){
            return from_.Allocate(size);
        }

        void SwapSpaces(){
            std::swap(from_, to_);
        }
    };

    class Object;
    class HeapMemoryIterator{
    private:
        uword current_;
        uword end_;
    public:
        HeapMemoryIterator(SinglespaceHeap* heap):
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