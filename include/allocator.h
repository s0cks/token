#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include <vector>
#include "common.h"
#include "object.h"

namespace Token{
    class RawObject;
    class ObjectPointerVisitor;

    typedef uintptr_t ObjectAddress;
    typedef std::map<ObjectAddress, RawObject*> ObjectAddressMap;

    class Allocator{
    private:
        static std::mutex mutex_;
        static ObjectAddressMap allocated_;
        static ObjectAddressMap roots_;

        static bool Unreference(RawObject* owner, RawObject* target, bool weak);
        static bool FinalizeObject(RawObject* obj);
        static bool IsRoot(RawObject* obj);
        static RawObject* GetObject(uintptr_t address);
        static RawObject* AllocateObject(uintptr_t size, Object::Type type);

        static inline RawObject* GetObject(void* ptr){
            return GetObject((uintptr_t)ptr);
        }

        Allocator() = delete;

        friend class Object;
        friend class MemorySweeper;
        friend class MemoryInformationSection;
    public:
        ~Allocator(){}

        static size_t GetNumberOfAllocatedObjects();
        static size_t GetNumberOfRootObjects();
        static uintptr_t GetBytesAllocated();
        static uintptr_t GetBytesFree();
        static uintptr_t GetTotalSize();

        static void Collect();
        static void* Allocate(uintptr_t size, Object::Type type=Object::Type::kUnknown);
        static void AddRoot(void* ptr);
        static void RemoveRoot(void* ptr);
        static bool AddStrongReference(void* object, void* target, void** ptr);
        static bool RemoveStrongReference(void* object, void* target);
        static bool AddWeakReference(void* object, void* target, void** ptr);
        static bool RemoveWeakReference(void* object, void* target);
        static bool PrintRoots();
        static bool PrintAllocated();
        static bool VisitAllocated(ObjectPointerVisitor* vis);
        static bool VisitRoots(ObjectPointerVisitor* vis);

        static inline bool IsRoot(void* ptr){
            return IsRoot(GetObject((ObjectAddress)ptr));
        }
    };
}

#endif //TOKEN_ALLOCATOR_H