#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include "common.h"
#include <vector>

namespace Token{
    class RawObject;
    class ObjectPointerVisitor;
    class Heap;
    class Allocator{
    private:
        static std::map<uintptr_t, RawObject*> allocated_;
        static std::map<uintptr_t, RawObject*> roots_;

        static Heap* GetEdenHeap();
        static bool Unreference(RawObject* owner, RawObject* target, bool weak);
        static RawObject* GetObject(void* ptr);
        static bool MoveObject(RawObject* src, RawObject* dst);

        Allocator() = delete;

        friend class MarkCopyScavenger;
        friend class Object;
    public:
        ~Allocator(){}

        static void* Allocate(uintptr_t size);
        static bool Collect();
        static bool AddRoot(void* ptr);

        static bool AddStrongReference(void* object, void* target, void** ptr);
        static bool RemoveStrongReference(void* object, void* target);

        static bool AddWeakReference(void* object, void* target, void** ptr);
        static bool RemoveWeakReference(void* object, void* target);

        static void* GetObject(uintptr_t address);

        static bool PrintRoots();
        static bool PrintAllocated();
        static bool VisitAllocated(ObjectPointerVisitor* vis);
        static bool VisitRoots(ObjectPointerVisitor* vis);
    };
}

#endif //TOKEN_ALLOCATOR_H