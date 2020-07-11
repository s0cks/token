#if defined(TOKEN_USE_KOA)
#include "scavenger.h"
#include "allocator.h"
#include "alloc/heap.h"

namespace Token{
    static inline Semispace* GetToSpace(){
        return Allocator::GetEdenHeap()->GetToSpace();
    }

    static inline Semispace* GetFromSpace(){
        return Allocator::GetEdenHeap()->GetFromSpace();
    }

    RawObject* Scavenger::EvacuateObject(RawObject* obj){
#ifdef TOKEN_DEBUG
        LOG(INFO) << "evacuating object: " << (*obj);
#endif//TOKEN_DEBUG
        RawObject* nobj = Allocator::AllocateObject(obj->GetObjectSize(), obj->GetObjectType());
        memcpy(nobj->GetObjectPointer(), obj->GetObjectPointer(), obj->GetObjectSize());
        obj->SetForwardingAddress(nobj->GetObjectAddress());
        return nobj;
    }

    bool Scavenger::ScavengeMemory(){
#ifdef TOKEN_DEBUG
        LOG(INFO) << "performing garbage collection...";
#endif//TOKEN_DEBUG

        DarkenRoots();
        MarkObjects();

        ObjectAddressMap allocated = Allocator::allocated_;
        ObjectAddressMap::iterator iter = allocated.begin();
        while(iter++ != allocated.end()){
            RawObject* obj = iter->second;
            if(!GetFromSpace()->Contains(obj->GetObjectAddress())){
                LOG(WARNING) << "object not in from space: " << (*obj);
                continue;
            }
            if(obj->IsGarbage()){
                LOG(WARNING) << "object is garbage: " << (*obj);
                Allocator::FinalizeObject(obj);
                iter = allocated.erase(iter);
                continue;
            }
            if(obj->IsForwarding()) continue;

            RawObject* nobj = EvacuateObject(obj);
            allocated.insert({ nobj->GetObjectAddress(), nobj });
        }

        Allocator::allocated_ = allocated;
#ifdef TOKEN_DEBUG
        LOG(INFO) << "garbage collection complete";
#endif//TOKEN_DEBUG
        return true;
    }
}

#endif//TOKEN_USE_KOA