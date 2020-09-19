#include "allocator.h"
#include "reference.h"

namespace Token{
    bool Reference::Accept(WeakObjectPointerVisitor* vis){
        return vis->Visit(slot_);
    }
}