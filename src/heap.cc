#include "heap.h"
#include "common.h"

namespace Token{
    Heap::Heap(uintptr_t init_cap):
            start_(nullptr),
            current_(nullptr){
        capacity_ = RoundUpPowTwo(init_cap);
        start_ = current_ = malloc(capacity_);
    }

    Heap::~Heap(){
        free(start_);
    }

#define CHUNK_SIZE(chunk) (*((uintptr_t*)(chunk)))
#define WITH_HEADER(chunk) ((void*)(((char*)chunk)-sizeof(uintptr_t)))
#define WITHOUT_HEADER(chunk) ((void*)(((char*)chunk)+sizeof(uintptr_t)))

    void* Heap::Alloc(uintptr_t size){
        size += sizeof(uintptr_t);
        if((Size() + size) > Capacity()) return nullptr;
        void* chunk = current_;
        current_ += size;
        memset(chunk, 0, size);
        CHUNK_SIZE(chunk) = (size-sizeof(uintptr_t));
        return WITHOUT_HEADER(chunk);
    }

    void Heap::Accept(Token::HeapVisitor* vis){
        void* chunk = start_;
        for(;;){
            if(!vis->VisitChunk(WITHOUT_HEADER(chunk), CHUNK_SIZE(chunk))){
                return;
            }
            chunk += CHUNK_SIZE(chunk)+sizeof(uintptr_t);
            if(CHUNK_SIZE(chunk) == 0) return;
        }
    }
}