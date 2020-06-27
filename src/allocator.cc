#include "crash_report.h"
#include "allocator.h"
#include "alloc/heap.h"
#include "alloc/reference.h"

#if defined(TOKEN_USE_CHENEYGC)
#include "alloc/mc_scavenger.h"
#else
#include "alloc/ms_scavenger.h"
#endif //TOKEN_USE_CHENEYGC

namespace Token{
    std::unordered_map<uintptr_t, RawObject*> Allocator::allocated_ = std::unordered_map<uintptr_t, RawObject*>();
    std::unordered_map<uintptr_t, RawObject*> Allocator::roots_ = std::unordered_map<uintptr_t, RawObject*>();

#if !defined(TOKEN_USE_CHENEYGC)
    uintptr_t Allocator::allocated_size_ = 0;
#endif//TOKEN_USE_CHENEYGC

#if defined(TOKEN_USE_CHENEYGC)
    Heap* Allocator::GetEdenHeap(){
        static Heap eden(FLAGS_minheap_size);
        return &eden;
    }

    bool Allocator::MoveObject(Token::RawObject *src, Token::RawObject *dst){
        auto pos = allocated_.find(src->GetObjectAddress());
        if(pos == allocated_.end()) return false;
        return allocated_.insert(std::make_pair(dst->GetObjectAddress(), dst)).second;
    }
#endif //TOKEN_USE_CHENEYGC

    void* Allocator::Allocate(uintptr_t size){
        if(size == 0){
            return nullptr;
        }
#if defined(TOKEN_USE_CHENEYGC)
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
#else
        uintptr_t total_size = size + sizeof(RawObject);
        if(total_size > GetBytesFree()){
            if(!Collect()) CrashReport::GenerateAndExit("Couldn't perform garbage collection");
            if(total_size > GetBytesFree()){
                std::stringstream ss;
                ss << "Couldn't allocate object of size: " << size;
                CrashReport::GenerateAndExit(ss.str());
            }
        }
        void* ptr = malloc(total_size);
        if(!ptr){
            if(!Collect()) CrashReport::GenerateAndExit("Couldn't perform garbage collection");
            if(!(ptr = malloc(total_size))){
                std::stringstream ss;
                ss << "Couldn't allocate object of size: " << size;
                CrashReport::GenerateAndExit(ss.str());
            }
        }
        memset(ptr, 0, total_size);
        RawObject* obj = new RawObject(Color::kWhite, size, ptr); //TODO: memory leak
#endif //TOKEN_USE_CHENEYGC
        allocated_.insert(std::make_pair(obj->GetObjectAddress(), obj));
        allocated_size_ += size;
#if defined(TOKEN_ENABLE_DEBUG)
        LOG(INFO) << "allocated object: " << (*obj);
#endif//TOKEN_ENABLE_DEBUG
        return obj->GetObjectPointer();
    }

    RawObject* Allocator::GetObject(uintptr_t address){
        auto pos = allocated_.find(address);
        if(pos == allocated_.end()) return nullptr;
        return pos->second;
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
        std::unordered_map<uintptr_t, RawObject*>::iterator iter = allocated_.begin();
        for(; iter != allocated_.end(); iter++){
            RawObject* obj = iter->second;
            if(!vis->Visit(obj)) return false;
        }
        return true;
    }

    bool Allocator::VisitRoots(ObjectPointerVisitor* vis){
        std::unordered_map<uintptr_t, RawObject*>::iterator iter = roots_.begin();
        for(; iter != roots_.end(); iter++){
            RawObject* obj = iter->second;
            if(!vis->Visit(obj)) return false;
        }
        return true;
    }

    bool Allocator::IsRoot(void* ptr){
        if(!ptr) return false;
        RawObject* obj = GetObject(ptr);
        if(obj == nullptr) return false;
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

#if !defined(TOKEN_USE_CHENEYGC)
    bool Allocator::DeleteObject(Token::RawObject* obj){
        uintptr_t address = obj->GetObjectAddress();

#if defined(TOKEN_ENABLE_DEBUG)
        LOG(INFO) << "removing object: " << (*obj);
#endif//TOKEN_ENABLE_DEBUG

        if(allocated_.erase(address) <= 0){
            std::stringstream ss;
            ss << "Couldn't remove object from allocator: " << (*obj);
            CrashReport::GenerateAndExit(ss.str());
        }
        allocated_size_ -= obj->GetObjectSize();
        delete obj;
        return true;
    }
#endif//TOKEN_USE_CHENEYGC

    bool Allocator::Collect(){
#if defined(TOKEN_USE_CHENEYGC)
        MarkCopyScavenger scavenger(GetEdenHeap());
#else
        MarkSweepScavenger scavenger;
#endif //TOKEN_USE_CHENEYGC
        return scavenger.ScavengeMemory();
    }

    size_t Allocator::GetBytesAllocated(){
#if defined(TOKEN_USE_CHENEYGC)
        return GetEdenHeap()->GetFromSpace()->GetAllocatedSize();
#else
        return allocated_size_;
#endif//TOKEN_USE_CHENEYGC
    }

    size_t Allocator::GetBytesFree(){
#if defined(TOKEN_USE_CHENEYGC)
        return GetEdenHeap()->GetFromSpace()->GetUnallocatedSize();
#else
        return FLAGS_minheap_size - allocated_size_;
#endif//TOKEN_USE_CHENEYGC
    }

    size_t Allocator::GetTotalSize(){
#if defined(TOKEN_USE_CHENEYGC)
        return GetEdenHeap()->GetSemispaceSize();
#else
        return FLAGS_minheap_size;
#endif//TOKEN_USE_CHENEYGC
    }
}