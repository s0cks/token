#ifndef TOKEN_ALLOCATOR_H
#define TOKEN_ALLOCATOR_H

#include <unordered_map>
#include <vector>
#include "common.h"

namespace Token{
    class RawObject;
    class ObjectPointerVisitor;
    class Heap;
    class Allocator{
    private:
        static std::unordered_map<uintptr_t, RawObject*> allocated_;
        static std::unordered_map<uintptr_t, RawObject*> roots_;
#if !defined(TOKEN_USE_CHENEYGC)
        static uintptr_t allocated_size_;

        static bool DeleteObject(RawObject* obj);
#endif//TOKEN_USE_CHENEYGC

        static bool Unreference(RawObject* owner, RawObject* target, bool weak);
        static RawObject* GetObject(uintptr_t address);

        static inline RawObject* GetObject(void* ptr){
            return GetObject((uintptr_t)ptr);
        }

#if defined(TOKEN_USE_CHENEYGC)
        static bool MoveObject(RawObject* src, RawObject* dst);
#endif //TOKEN_USE_CHENEYGC

        Allocator() = delete;

        friend class MarkCopyScavenger;
        friend class MarkSweepScavenger;
        friend class Object;
        friend class ObjectScopeHandle;
        friend class MemoryInformationSection;
    public:
        ~Allocator(){}

        static size_t GetNumberOfAllocatedObjects(){
            return allocated_.size();
        }

        static uintptr_t GetBytesAllocated();
        static uintptr_t GetBytesFree();
        static uintptr_t GetTotalSize();

        static void* Allocate(uintptr_t size);
        static void AddRoot(void* ptr);
        static void RemoveRoot(void* ptr);
        static bool IsRoot(void* ptr);
        static bool Collect();
        static bool AddStrongReference(void* object, void* target, void** ptr);
        static bool RemoveStrongReference(void* object, void* target);

        static bool AddWeakReference(void* object, void* target, void** ptr);
        static bool RemoveWeakReference(void* object, void* target);

        static bool PrintRoots();
        static bool PrintAllocated();
        static bool VisitAllocated(ObjectPointerVisitor* vis);
        static bool VisitRoots(ObjectPointerVisitor* vis);


#if defined(TOKEN_USE_CHENEYGC)
        static Heap* GetEdenHeap();
#endif
    };

    /*
     * TODO
    class ObjectScopeHandle{
    private:
        std::vector<uintptr_t> allocated_;
    public:
        ObjectScopeHandle():
            allocated_(){}
        ~ObjectScopeHandle(){
            for(auto& it : allocated_){
                RawObject* obj;
                if(!(obj = Allocator::GetObject(it))){
                    std::stringstream ss;
                    ss << "Couldn't get object from allocator @" << std::hex << it;
                    CrashReport::GenerateAndExit(ss.str());
                }
                Allocator::RemoveRoot(obj->GetObjectPointer());
            }
        }

        void* Allocate(uintptr_t size){
            void* ptr = Allocator::Allocate(size);
            Allocator::AddRoot(ptr);
            return ptr;
        }
    };
     */
}

#endif //TOKEN_ALLOCATOR_H