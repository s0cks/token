#ifndef TOKEN_SCAVENGER_H
#define TOKEN_SCAVENGER_H

#include "heap.h"

namespace Token{
    class Scavenger{
    private:
        Heap* heap_;
        Semispace from_space_;
        Semispace to_space_;

        Scavenger(Heap* heap):
            heap_(heap),
            from_space_(heap->GetFromSpace()),
            to_space_(heap->GetToSpace()){}

        void ScavengeMemory();
    public:
        ~Scavenger(){}

        Heap* GetHeap() const{
            return heap_;
        }

        Semispace GetFromSpace() const{
            return from_space_;
        }

        Semispace GetToSpace() const{
            return to_space_;
        }

        static void Scavenge(Heap* heap){
            Scavenger scavenger(heap);
            scavenger.ScavengeMemory();
        }
    };
}

#endif //TOKEN_SCAVENGER_H