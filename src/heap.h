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
        friend class HeapDumpWriter;
        friend class HeapDumpReader;
    private:
        std::recursive_mutex mutex_;
        Semispace from_space_;
        Semispace to_space_;
        size_t size_;
        size_t semi_size_;

        void SetAllocatedSize(size_t size){
            from_space_.current_ = (from_space_.start_ + size);
        }
    public:
        Heap(uword start, size_t heap_size, size_t semispace_size):
            mutex_(),
            size_(heap_size),
            semi_size_(semispace_size),
            from_space_(start, semispace_size),
            to_space_(start + semispace_size, semispace_size){}
        ~Heap(){}

        Semispace* GetFromSpace(){
            return &from_space_;
        }

        Semispace* GetToSpace(){
            return &to_space_;
        }

        uword GetStartAddress() const{
            return from_space_.GetStartAddress();
        }

        uword GetEndAddress() const{
            return GetStartAddress() + GetTotalSize();
        }

        bool Contains(uword address) const{
            return address >= GetStartAddress() &&
                   address <= GetEndAddress();
        }

        size_t GetSemispaceSize() const{
            return semi_size_;
        }

        size_t GetTotalSize() const{
            return size_;
        }

        size_t GetAllocatedSize(){
            std::lock_guard<std::recursive_mutex> guard(mutex_);
            return from_space_.GetAllocatedSize();
        }

        size_t GetUnallocatedSize(){
            std::lock_guard<std::recursive_mutex> guard(mutex_);
            return from_space_.GetUnallocatedSize();
        }

        void SwapSpaces(){
            std::lock_guard<std::recursive_mutex> guard(mutex_);
            std::swap(from_space_, to_space_);
        }

        void* Allocate(size_t size){
            std::lock_guard<std::recursive_mutex> guard(mutex_);
            return from_space_.Allocate(size);
        }

        bool Accept(ObjectPointerVisitor* vis);
    };
}

#endif //TOKEN_HEAP_H