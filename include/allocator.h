#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include <vector>
#include <condition_variable>
#include "common.h"
#include "object.h"

namespace Token{
    //TODO
    // - allocation stats
    // - zoning
    // - add allocation + GC debug information
    // - refactor scavenge policies into Minor + Major Policy classes
    // - refactor stack allocations
    // - locking for allocations + collections
    class Semispace;
    class SemispaceHeap;
    class SinglespaceHeap;
    class Allocator{
        friend class Object;
        friend class MemoryInformationSection;
    private:
        Allocator() = delete;

        template<typename I>
        static void MarkObjects(Iterable<I> iter);

        template<typename I>
        static void FinalizeObjects(Iterable<I> iter);

        template<typename I>
        static void CopyLiveObjects(Iterable<I> iter);

        static void EvacuateEdenObjects();
        static void EvacuateSurvivorObjects();
        static void EvacuateTenuredObjects();

        template<bool asRoot, typename I>
        static void NotifyWeakReferences(Iterable<I> iter);

        template<typename I>
        static void UpdateNonRootReferences(Iterable<I> iter);

        static void UpdateStackReferences();

        static void Initialize(Object* obj);
        static void UntrackStackObject(Object* obj);
    public:
        ~Allocator(){}

        static void Initialize();
        static void MinorGC();
        static void MajorGC();

        static void* Allocate(size_t size, Type type=Type::kUnknownType);
        static SinglespaceHeap* GetEdenHeap();
        static SemispaceHeap* GetSurvivorHeap();
        static SinglespaceHeap* GetTenuredHeap();
        static Semispace* GetSurvivorFromSpace();
        static Semispace* GetSurvivorToSpace();
    };
}

#endif //TOKEN_ALLOCATOR_H