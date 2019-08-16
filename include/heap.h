#ifndef TOKEN_HEAP_H
#define TOKEN_HEAP_H

#include <cstdint>
#include <cstring>

namespace Token{
    class HeapVisitor;

    class Heap{
    private:
        void* start_;
        void* current_;
        uintptr_t capacity_;
    public:
        Heap(uintptr_t init_cap);
        ~Heap();

        uintptr_t Capacity() const{
            return capacity_;
        }

        uintptr_t Size() const{
            return (((char*)current_) - ((char*)start_));
        }

        void* Alloc(uintptr_t size);
        void Accept(HeapVisitor* vis);
    };

    class HeapVisitor{
    public:
        HeapVisitor(){}
        virtual ~HeapVisitor() = default;

        virtual bool VisitChunk(void* ptr, size_t size) = 0;
    };
}

#endif //TOKEN_HEAP_H