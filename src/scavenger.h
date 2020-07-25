#ifndef TOKEN_SCAVENGER_H
#define TOKEN_SCAVENGER_H

#include "heap.h"

namespace Token{
    class Scavenger{
    public:
        static const size_t kNumberOfCollectionsForPromotion = 3;
    private:
        Scavenger() = delete;

        template<typename I>
        static void MarkObjects(Iterable<I> iterable);

        template<typename I>
        static void FinalizeObjects(Iterable<I> iterable);

        template<typename I>
        static void RelocateObjects(Iterable<I> iterable);

        template<bool AsRoot, typename I>
        static void NotifyWeakReferences(Iterable<I> iterable);

        template<typename I>
        static void UpdateNonRootReferences(Iterable<I> iterable);

        template<typename I>
        static void CopyLiveObjects(Iterable<I> iterable);
    public:
        ~Scavenger() = delete;

        static void Scavenge();
    };
}

#endif //TOKEN_SCAVENGER_H