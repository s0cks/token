#ifndef TOKEN_SCAVENGER_H
#define TOKEN_SCAVENGER_H

#include "heap.h"
#include "allocator.h"

namespace Token{
    class Scavenger{
    private:
        Scavenger() = delete;

        static inline Heap*
        GetHeap(){
            return Allocator::GetHeap();
        }

        static inline Semispace*
        GetFromSpace(){
            return GetHeap()->GetFromSpace();
        }

        static inline Semispace*
        GetToSpace(){
            return GetHeap()->GetToSpace();
        }
    public:
        ~Scavenger() = delete;

        static bool Scavenge();
    };
}

#endif //TOKEN_SCAVENGER_H