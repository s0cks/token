#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include <vector>
#include <condition_variable>
#include "common.h"
#include "object.h"

namespace Token{
    class Heap;
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

        static void EvacuateLiveObjects(Heap* src, Heap* dst);

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

        static void* Allocate(size_t size, Object::Type type=Object::kUnknown);
        static Heap* GetEdenHeap();
        static Heap* GetSurvivorFromSpace();
        static Heap* GetSurvivorToSpace();
    };
}

#endif //TOKEN_ALLOCATOR_H