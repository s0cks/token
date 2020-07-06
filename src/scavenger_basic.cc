#if !defined(TOKEN_USE_KOA)
#include "alloc/scavenger.h"
#include "allocator.h"

namespace Token{
    class MemorySweeper : public ObjectPointerVisitor{
    public:
        MemorySweeper(): ObjectPointerVisitor(){}
        ~MemorySweeper(){}

        bool Visit(RawObject* obj){
            if(obj->IsGarbage()) return Allocator::FinalizeObject(obj);
            return true; // skip object, not garbage
        }
    };

    bool Scavenger::ScavengeMemory(){
#ifdef TOKEN_DEBUG
        LOG(INFO) << "performing garbage collection...";
        uintptr_t initial_size = Allocator::GetBytesAllocated();
#endif//TOKEN_DEBUG
        DarkenRoots();
        MarkObjects();

        MemorySweeper sweeper;
        Allocator::VisitAllocated(&sweeper);
#ifdef TOKEN_DEBUG
        uintptr_t total_mem_scavenged = (initial_size - Allocator::GetBytesAllocated());
        LOG(INFO) << "garbage collection complete";
        LOG(INFO) << "collected: " << total_mem_scavenged << "/" << Allocator::GetTotalSize() << " bytes";
        LOG(INFO) << "allocated: " << Allocator::GetBytesAllocated() << "/" << Allocator::GetTotalSize() << " bytes";
#endif//TOKEN_DEBUG
        return true;
    }
}

#endif//TOKEN_USE_KOA