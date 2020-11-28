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
        intptr_t size_;
    public:
        Semispace():
            start_(0),
            current_(0),
            size_(0){}
        Semispace(uword start, intptr_t size):
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

        intptr_t GetAllocatedSize() const{
            return GetCurrentAddress() - GetStartAddress();
        }

        intptr_t GetUnallocatedSize() const{
            return GetEndAddress() - GetCurrentAddress();
        }

        intptr_t GetTotalSize() const{
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

        void* Allocate(intptr_t size);
        void Accept(ObjectPointerVisitor* vis);

        Semispace& operator=(const Semispace& other){
            start_ = other.start_;
            current_ = other.current_;
            size_ = other.size_;
            return (*this);
        }
    };

    class Heap{
    private:
        std::mutex mutex_;
        Space space_;
        uword start_;
        intptr_t size_;
        intptr_t semispace_size_;
        Semispace from_space_;
        Semispace to_space_;

        void SetAllocatedSize(intptr_t size){
            from_space_.current_ = (from_space_.start_ + size);
        }
    public:
        Heap(Space space, uword start, intptr_t heap_size, intptr_t semispace_size):
            mutex_(),
            space_(space),
            start_(start),
            size_(heap_size),
            semispace_size_(semispace_size),
            from_space_(start, semispace_size),
            to_space_(start + semispace_size, semispace_size){}
        ~Heap(){}

        Space GetSpace() const{
            return space_;
        }

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

        intptr_t GetSemispaceSize() const{
            return semispace_size_;
        }

        intptr_t GetTotalSize() const{
            return size_;
        }

        intptr_t GetAllocatedSize(){
            std::lock_guard<std::mutex> guard(mutex_);
            return from_space_.GetAllocatedSize();
        }

        intptr_t GetUnallocatedSize(){
            std::lock_guard<std::mutex> guard(mutex_);
            return from_space_.GetUnallocatedSize();
        }

        void SwapSpaces(){
            std::lock_guard<std::mutex> guard(mutex_);
            std::swap(from_space_, to_space_);
        }

        void* Allocate(intptr_t size){
            std::lock_guard<std::mutex> guard(mutex_);
            return from_space_.Allocate(size);
        }

        bool VisitObjects(ObjectPointerVisitor* vis);
        bool VisitMarkedObjects(ObjectPointerVisitor* vis);
    };
}

#endif//TOKEN_HEAP_H