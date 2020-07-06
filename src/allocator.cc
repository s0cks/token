#include "allocator.h"
#include "alloc/reference.h"
#include "alloc/scavenger.h"
#include "crash_report.h"

namespace Token{
    ObjectAddressMap Allocator::allocated_ = ObjectAddressMap();
    ObjectAddressMap Allocator::roots_ = ObjectAddressMap();

    size_t Allocator::GetNumberOfRootObjects(){
        return roots_.size();
    }

    size_t Allocator::GetNumberOfAllocatedObjects() {
        return allocated_.size();
    }

    RawObject* Allocator::GetObject(uintptr_t address){
        auto pos = allocated_.find(address);
        if(pos == allocated_.end()) return nullptr;
        return pos->second;
    }

    void Allocator::Collect(){
        if(!Scavenger::ScavengeMemory()) CrashReport::GenerateAndExit("Couldn't perform garbage collection");
    }

    void* Allocator::Allocate(uintptr_t size, Object::Type type){
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
        ObjectAddressMap::iterator iter = allocated_.begin();
        for(; iter != allocated_.end(); iter++){
            RawObject* obj = iter->second;
            if(!vis->Visit(obj)) return false;
        }
        return true;
    }

    bool Allocator::VisitRoots(ObjectPointerVisitor* vis){
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
        if(!obj) return false;
        auto pos = roots_.find(obj->GetObjectAddress());
        return pos != roots_.end();
    }

    void Allocator::RemoveRoot(void* ptr){
        if(!ptr) return;
        RawObject* obj = GetObject(ptr);
        if(obj == nullptr) CrashReport::GenerateAndExit("Attempt to remove root to non-managed object");
        if(!roots_.erase((uintptr_t)ptr)){
            std::stringstream ss;
            ss << "Couldn't remove root for object: " << obj;
            CrashReport::GenerateAndExit(ss.str());
        }
    }

    void Allocator::AddRoot(void* ptr){
        if(!ptr) return;
        RawObject* obj = GetObject(ptr);
        if(obj == nullptr) CrashReport::GenerateAndExit("Attempt to add root to non-managed object");
        if(!roots_.insert({ obj->GetObjectAddress(), obj }).second){
            std::stringstream ss;
            ss << "Couldn't add root for object: " << obj;
            CrashReport::GenerateAndExit(ss.str());
        }
    }

    bool Allocator::Unreference(Token::RawObject *owner, Token::RawObject *target, bool weak){
        for(RawObject::ReferenceList::iterator iter = owner->owned_.begin(); iter != owner->owned_.end(); iter++){
            Reference* ref = (*iter);
            if(ref->GetTarget() != target) continue;
            if(ref->IsWeakReference() != weak) continue;
            return true;
        }
        return false;
    }

    bool Allocator::AddStrongReference(void *object, void *target, void **ptr){
        RawObject* src = GetObject(object);
        if(src == nullptr){
            LOG(WARNING) << "couldn't find src";
            return false;
        }
        RawObject* dst = GetObject(target);
        if(dst == nullptr){
            LOG(WARNING) << "couldn't find dst";
            return false;
        }
        StrongReference* ref = new StrongReference(src, dst, ptr);
        src->owned_.push_back(ref);
        dst->pointing_.push_back(ref);
        return true;
    }

    bool Allocator::RemoveStrongReference(void *object, void *target){
        RawObject* src = GetObject(object);
        RawObject* dst = GetObject(target);
        return Unreference(src, dst, false);
    }

    bool Allocator::AddWeakReference(void *object, void *target, void **ptr){
        RawObject* src = GetObject(object);
        if(!src) return false;
        RawObject* dst = GetObject(target);
        if(!dst) return false;

        WeakReference* ref = new WeakReference(src, dst, ptr);
        src->owned_.push_back(ref);
        dst->pointing_.push_back(ref);
        return true;
    }

    bool Allocator::RemoveWeakReference(void *object, void *target){
        RawObject* src = GetObject(object);
        RawObject* dst = GetObject(target);
        return Unreference(src, dst, true);
    }
}