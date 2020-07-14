#include "allocator.h"
#include "alloc/reference.h"
#include "alloc/scavenger.h"
#include "crash_report.h"

#include "block_miner.h"

#ifdef TOKEN_USE_KOA
#include "alloc/heap.h"
#endif//TOKEN_USE_KOA

namespace Token{
    std::recursive_mutex Allocator::mutex_;
    ObjectAddressMap Allocator::allocated_ = ObjectAddressMap();
    ObjectAddressMap Allocator::roots_ = ObjectAddressMap();

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_)
#define LOCK std::unique_lock<std::recursive_mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

#ifdef TOKEN_USE_KOA
    static Heap* eden_ = nullptr;

    Heap* Allocator::GetEdenHeap(){
        return eden_;
    }
#endif//TOKEN_USE_KOA

    void Allocator::Initialize(){
#ifdef TOKEN_USE_KOA
        LOCK_GUARD;
        eden_ = new Heap(FLAGS_minheap_size);
#endif//TOKEN_USE_KOA
    }

    size_t Allocator::GetNumberOfRootObjects(){
        LOCK_GUARD;
        return roots_.size();
    }

    size_t Allocator::GetNumberOfAllocatedObjects() {
        LOCK_GUARD;
        return allocated_.size();
    }

    RawObject* Allocator::GetObject(uintptr_t address){
        LOCK_GUARD;
        auto pos = allocated_.find(address);
        if(pos == allocated_.end()) return nullptr;
        return pos->second;
    }

    void Allocator::Collect(){
        LOCK_GUARD;
        BlockMiner::Pause();
        if(!Scavenger::ScavengeMemory()) CrashReport::GenerateAndExit("Couldn't perform garbage collection");
        BlockMiner::Resume();
    }

    void* Allocator::Allocate(uintptr_t size, Object::Type type){
        LOCK_GUARD;
        if(size == 0) return nullptr;
        uintptr_t total_size = size + sizeof(RawObject);
        if(total_size > GetBytesFree()){
            Collect();
            if(total_size > GetBytesFree()){
                std::stringstream ss;
                ss << "Couldn't allocate object of size: " << size;
                CrashReport::GenerateAndExit(ss);
            }
        }
        RawObject* obj = AllocateObject(total_size, type);
        allocated_.insert({ obj->GetObjectAddress(), obj });
        return obj->GetObjectPointer();
    }

    bool Allocator::VisitAllocated(ObjectPointerVisitor* vis){
        LOCK_GUARD;
        ObjectAddressMap::iterator iter = allocated_.begin();
        for(; iter != allocated_.end(); iter++){
            RawObject* obj = iter->second;
            if(!vis->Visit(obj)) return false;
        }
        return true;
    }

    bool Allocator::VisitRoots(ObjectPointerVisitor* vis){
        LOCK_GUARD;
        ObjectAddressMap::iterator iter = roots_.begin();
        for(; iter != roots_.end(); iter++){
            RawObject* obj = iter->second;
            if(!vis->Visit(obj)) return false;
        }
        return true;
    }

    class GCObjectPrinter : public ObjectPointerVisitor{
    public:
        GCObjectPrinter(): ObjectPointerVisitor(){}
        ~GCObjectPrinter(){}

        bool Visit(RawObject* obj){
            LOG(INFO) << (*obj);
            return true;
        }
    };

    bool Allocator::PrintAllocated(){
        GCObjectPrinter printer;
        return VisitAllocated(&printer);
    }

    bool Allocator::PrintRoots(){
        GCObjectPrinter printer;
        return VisitRoots(&printer);
    }

    bool Allocator::IsRoot(RawObject* obj){
        LOCK_GUARD;
        if(!obj) return false;
        auto pos = roots_.find(obj->GetObjectAddress());
        return pos != roots_.end();
    }

    void Allocator::RemoveRoot(void* ptr){
        LOCK_GUARD;
        if(!ptr || !IsRoot(ptr)) return;
        RawObject* obj = GetObject(ptr);
        if(obj == nullptr) CrashReport::GenerateAndExit("Attempt to remove root to non-managed object");
        if(!roots_.erase((uintptr_t)ptr)){
            std::stringstream ss;
            ss << "Couldn't remove root for object: " << obj;
            CrashReport::GenerateAndExit(ss.str());
        }
    }

    void Allocator::AddRoot(void* ptr){
        LOCK_GUARD;
        if(!ptr || IsRoot(ptr)) return;
        RawObject* obj = GetObject(ptr);
        if(obj == nullptr) CrashReport::GenerateAndExit("Attempt to add root to non-managed object");
        if(!roots_.insert({ obj->GetObjectAddress(), obj }).second){
            std::stringstream ss;
            ss << "Couldn't add root for object: " << obj;
            CrashReport::GenerateAndExit(ss.str());
        }
    }

    bool Allocator::Unreference(Token::RawObject *owner, Token::RawObject *target, bool weak){
        LOCK_GUARD;
        for(RawObject::ReferenceList::iterator iter = owner->owned_.begin(); iter != owner->owned_.end(); iter++){
            Reference* ref = (*iter);
            if(ref->GetTarget()->GetObjectAddress() != target->GetObjectAddress()) continue;
            if(ref->IsWeakReference() != weak) continue;
            return true;
        }
        return false;
    }

    bool Allocator::AddStrongReference(void *object, void *target, void **ptr){
        LOCK_GUARD;
        RawObject* src = nullptr;
        if(!(src =  GetObject(object))){
            LOG(WARNING) << "couldn't find src";
            return false;
        }
        RawObject* dst = nullptr;
        if(!(dst = GetObject(target))){
            LOG(WARNING) << "couldn't find dst";
            return false;
        }
        StrongReference* ref = new StrongReference(src, dst, ptr);
        src->owned_.push_back(ref);
        dst->pointing_.push_back(ref);
        return true;
    }

    bool Allocator::RemoveStrongReference(void *object, void *target){
        LOCK_GUARD;
        RawObject* src = nullptr;
        if(!(src = GetObject(object))){
            LOG(WARNING) << "couldn't find src";
            return false;
        }
        RawObject* dst = nullptr;
        if(!(dst = GetObject(target))){
            LOG(WARNING) << "couldn't find dst";
            return false;
        }
        return Unreference(src, dst, false);
    }

    bool Allocator::AddWeakReference(void *object, void *target, void **ptr){
        LOCK_GUARD;
        RawObject* src = nullptr;
        if(!(src = GetObject(object))){
            LOG(WARNING) << "couldn't find src";
            return false;
        }
        RawObject* dst = nullptr;
        if(!(dst = GetObject(target))){
            LOG(WARNING) << "couldn't find dst";
            return false;
        }

        WeakReference* ref = new WeakReference(src, dst, ptr);
        src->owned_.push_back(ref);
        dst->pointing_.push_back(ref);
        return true;
    }

    bool Allocator::RemoveWeakReference(void *object, void *target){
        LOCK_GUARD;
        RawObject* src = nullptr;
        if(!(src = GetObject(object))){
            LOG(WARNING) << "couldn't find src";
            return false;
        }
        RawObject* dst = nullptr;
        if(!(dst = GetObject(target))){
            LOG(WARNING) << "couldn't find dst";
            return false;
        }
        return Unreference(src, dst, true);
    }

    bool Allocator::Finalize(RawObject* obj){
        LOCK_GUARD;
        if(IsRoot(obj)){
            LOG(WARNING) << "cannot finalize root object: " << (*obj);
            return false;
        }

#ifdef TOKEN_DEBUG
        LOG(INFO) << "finalizing object: " << (*obj);
#endif//TOKEN_DEBUG
        Object* o = (Object*)obj->GetObjectPointer();
        if(!o->Finalize()){
            std::stringstream ss;
            ss << "Couldn't finalize object: " << (*obj);
            CrashReport::GenerateAndExit(ss);
        }

        return FinalizeObject(obj);
    }
}