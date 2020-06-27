#ifndef TOKEN_MC_SCAVENGER_H
#define TOKEN_MC_SCAVENGER_H
#if defined(TOKEN_USE_CHENEYGC)

#include "scavenger.h"
#include "heap.h"

namespace Token{
    class MarkCopyScavenger : public Scavenger{
    private:
        Heap* heap_;
    public:
        MarkCopyScavenger(Heap* heap):
            Scavenger(),
            heap_(heap){}
        ~MarkCopyScavenger(){}

        Heap* GetHeap() const{
            return heap_;
        }

        Semispace* GetFromSpace() const{
            return GetHeap()->GetFromSpace();
        }

        Semispace* GetToSpace() const{
            return GetHeap()->GetToSpace();
        }

        RawObject* Evacuate(RawObject* obj);
        bool ScavengeMemory();
    };
}

#endif //TOKEN_USE_CHENEYGC
#endif //TOKEN_MC_SCAVENGER_H