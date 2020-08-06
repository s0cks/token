#include "vthread.h"
#include "allocator.h"

namespace Token{
    void Thread::WriteBarrier(RawObject** slot, RawObject* value){
        if(value) value->IncrementReferenceCount();
        if((*slot))(*slot)->DecrementReferenceCount();
        (*slot) = value;
    }
}