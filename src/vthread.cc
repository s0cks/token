#include "vthread.h"
#include "object.h"
#include "alloc/allocator.h"

namespace Token{
    void Thread::WriteBarrier(Object** slot, Object* value){
        if(value) value->IncrementReferenceCount();
        if((*slot))(*slot)->DecrementReferenceCount();
        (*slot) = value;
    }
}