#include "allocator.h"
#include "alloc/heap.h"
#include "alloc/scavenger.h"
#include "alloc/reference.h"

namespace Token{
    std::map<uintptr_t, RawObject*> Allocator::allocated_ = std::map<uintptr_t, RawObject*>();
    std::map<uintptr_t, RawObject*> Allocator::roots_ = std::map<uintptr_t, RawObject*>();

    Heap* Allocator::GetEdenHeap(){
        static Heap eden(FLAGS_minheap_size);
        return &eden;
    }

    void* Allocator::Allocate(uintptr_t size){
        RawObject* obj = nullptr;
        if(!(obj = GetEdenHeap()->Allocate(size))){
            if(!Collect()){
                LOG(WARNING) << "couldn't collect garbage objects from eden space";
                return nullptr;
            }

            if(!(obj = GetEdenHeap()->Allocate(size))) {
                LOG(WARNING) << "couldn't allocate space for object of size: " << size;
                return nullptr;
            }
        }
        LOG(INFO) << "allocated object: " << (*obj);
        allocated_.insert(std::make_pair(obj->GetObjectAddress(), obj));
        return obj->GetObjectPointer();
    }

    RawObject* Allocator::GetObject(void* ptr){
        if(!ptr) return nullptr;
        auto pos = allocated_.find((uintptr_t)(ptr));
        if(pos == allocated_.end()) return nullptr;
        return pos->second;
    }

    void* Allocator::GetObject(uintptr_t address){
        auto pos = allocated_.find(address);
        if(pos == allocated_.end()) return nullptr;
        RawObject* obj = pos->second;
        if(obj->IsForwarding()){
            return (void*)obj->GetForwardingAddress();
        } else{
            return (void*)obj->GetObjectAddress();
        }
    }

    bool Allocator::MoveObject(Token::RawObject *src, Token::RawObject *dst){
        auto pos = allocated_.find(src->GetObjectAddress());
        if(pos == allocated_.end()) return false;
        return allocated_.insert(std::make_pair(dst->GetObjectAddress(), dst)).second;
    }

    class GCObjectPrinter : public ObjectPointerVisitor{
    public:
        GCObjectPrinter(): ObjectPointerVisitor(){}
        ~GCObjectPrinter(){}

        bool Visit(RawObject* obj){
            LOG(INFO) << " - " << (*obj);
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

    bool Allocator::VisitAllocated(ObjectPointerVisitor* vis){
        std::map<uintptr_t, RawObject*>::iterator iter = allocated_.begin();
        for(; iter != allocated_.end(); iter++){
            RawObject* obj = iter->second;
            if(!vis->Visit(obj)) return false;
        }
        return true;
    }

    bool Allocator::VisitRoots(ObjectPointerVisitor* vis){
        std::map<uintptr_t, RawObject*>::iterator iter = roots_.begin();
        for(; iter != roots_.end(); iter++){
            RawObject* obj = iter->second;
            if(!vis->Visit(obj)) return false;
        }
        return true;
    }

    bool Allocator::AddRoot(void* ptr){
        if(!ptr) return false;
        RawObject* obj = GetObject(ptr);
        return roots_.insert(std::make_pair((uintptr_t)ptr, obj)).second;
    }

    bool Allocator::Unreference(Token::RawObject *owner, Token::RawObject *target, bool weak){
        for(RawObject::ReferenceList::iterator iter = owner->owned_.begin(); iter != owner->owned_.end(); iter++){
            Reference* ref = (*iter).get();
            if(ref->GetTarget() != target) continue;
            if(ref->IsWeakReference() != weak) continue;
            return true;
        }
        return false;
    }

    bool Allocator::AddStrongReference(void *object, void *target, void **ptr){
        RawObject* src = GetObject(object);
        if(!src) return false;
        RawObject* dst = GetObject(target);
        if(!dst) return false;

        StrongReference* ref = new StrongReference(src, dst, ptr);
        src->owned_.push_back(std::unique_ptr<Reference>(ref));
        dst->pointing_.push_back(std::unique_ptr<Reference>(ref));
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
        src->owned_.push_back(std::unique_ptr<Reference>(ref));
        dst->pointing_.push_back(std::unique_ptr<Reference>(ref));
        return true;
    }

    bool Allocator::RemoveWeakReference(void *object, void *target){
        RawObject* src = GetObject(object);
        RawObject* dst = GetObject(target);
        return Unreference(src, dst, true);
    }

    bool Allocator::Collect(){
        return Scavenger::Scavenge(GetEdenHeap());
    }
}