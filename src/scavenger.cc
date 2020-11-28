#include "heap.h"
#include "scavenger.h"
#include "server.h"
#include "http/healthcheck.h"

namespace Token{
    static inline std::string
    ObjectLocation(Object* obj){
        std::stringstream ss;
        ss << "[" << std::hex << obj << ":" << std::dec << obj->GetSize() << "]";
        return ss.str();
    }

    class Marker{
    protected:
        std::vector<uword>& stack_;

        void MarkObject(Object* obj){
            stack_.push_back((uword)obj);
        }
    public:
        Marker(std::vector<uword>& stack):
            stack_(stack){}
        virtual ~Marker() = default;
    };

    class RootObjectMarker : public Marker, public WeakObjectPointerVisitor{
    public:
        RootObjectMarker(std::vector<uword>& stack):
            Marker(stack),
            WeakObjectPointerVisitor(){}
        ~RootObjectMarker() = default;

        bool Visit(Object** obj){
            Object* data = (*obj);
            MarkObject(data);
            return true;
        }
    };

    bool Scavenger::ScavengeObject(Object* obj){
        intptr_t size = obj->GetSize();

        Object* nobj = (Object*)Allocator::GetNewHeap()->GetToSpace()->Allocate(size);
        nobj->SetType(obj->GetType());
        nobj->SetSize(obj->GetSize());
        LOG(INFO) << "relocating object: " << ObjectLocation(obj) << " => " << ObjectLocation(nobj);

        obj->ptr_ = (uword)nobj;
        obj->IncrementCollectionsCounter();
        new_stats_.num_survived_++;
        new_stats_.bytes_survived_ += size;
        return true;
    }

    bool Scavenger::PromoteObject(Object* obj){
        LOG(INFO) << "promoting object " << ObjectLocation(obj);
        intptr_t size = obj->GetSize();

        void* nptr = Allocator::GetOldHeap()->Allocate(size);
        obj->ptr_ = (uword)nptr;

        new_stats_.num_promoted_++;
        new_stats_.bytes_promoted_ += size;
        return true;
    }

    bool Scavenger::FinalizeObject(Object* obj){
        LOG(INFO) << "finalizing object " << ObjectLocation(obj);
        intptr_t size = obj->GetSize();

        obj->~Object(); //TODO: Call finalizer for obj
        obj->ptr_ = 0;

        new_stats_.num_collected_++;
        new_stats_.bytes_collected_ += size;
        return true;
    }

    bool Scavenger::ProcessRoots(){
        SetPhase(Phase::kMarkPhase);

        std::vector<uword> work;
        RootObjectMarker marker(work);
        {
            // Mark the active peers
            LOG(INFO) << "marking the server peer sessions....";
            if(!Server::Accept(&marker)){
                LOG(WARNING) << "couldn't visit the current peer sessions.";
                return false;
            }
        }
        {
            // Mark the active http sessions
            LOG(INFO) << "marking the active http sessions....";
            if(!HealthCheckService::Accept(&marker)){
                LOG(WARNING) << "couldn't visit the http sessions.";
                return false;
            }
        }
        {
            // Mark the Stack Roots
            LOG(INFO) << "marking the stack roots....";
            if(!HandleBase::VisitHandles(&marker)){
                LOG(WARNING) << "couldn't visit current handles.";
                return false;
            }
        }

        while(!work.empty()){
            Object* obj = (Object*)work.back();
            work.pop_back();
            if (!obj->IsMarked() && Allocator::GetNewHeap()->Contains((uword)obj)) {
                LOG(INFO) << "marking object " << ObjectLocation(obj);
                obj->SetMarked();
                if (!obj->Accept(&marker)) {
                    LOG(WARNING) << "couldn't visit object references.";
                    continue;
                }
            }
        }

        return true;
    }

    class ReferenceNotifier : public WeakObjectPointerVisitor,
                              public ObjectPointerVisitor{
    public:
        bool Visit(Object* obj){
            return obj->Accept(this);
        }

        bool Visit(Object** field){
            Object* obj = (*field);
            if(!obj || obj->IsMarked()){
                return true;
            }
            if(!obj->ptr_){
                LOG(INFO) << "notifying: " << ObjectLocation(obj);
                (*field) = nullptr;
            }
            return true;
        }
    };

    static inline void
    objcpy(void* dst, const void* src, intptr_t size){
        uword* __restrict dst_cursor = (uword*)dst;
        const uword* __restrict src_cursor = (uword*)src;
        do{
            uword a = *src_cursor++;
            uword b = *src_cursor++;
            *dst_cursor++ = a;
            *dst_cursor++ = b;
            size -= (2 * sizeof(uword));
        } while(size > 0);
    }

    class ReferenceUpdater : public ObjectPointerVisitor, public WeakObjectPointerVisitor{
    public:
        bool Visit(Object* obj){
            if(!obj || !obj->IsMarked()) return true;
            LOG(INFO) << "updating " << ObjectLocation(obj);
            memcpy((void*)obj->ptr_, (void*)(obj + sizeof(Object)), obj->GetSize()-sizeof(Object));
            obj->ClearMarked();
            return true;
        }

        bool Visit(Object** field){
            return Visit(*field);
        }
    };

    class ObjectSweeper : public ObjectPointerVisitor{
    private:
        Scavenger* parent_;
    public:
        ObjectSweeper(Scavenger* parent):
            ObjectPointerVisitor(),
            parent_(parent){}
        ~ObjectSweeper() = default;

        Scavenger* GetParent() const{
            return parent_;
        }

        bool Visit(Object* obj){
            if(obj->IsMarked()) return true; // skip
            GetParent()->FinalizeObject(obj);
            return true;
        }
    };

    bool Scavenger::ScavengeMemory(){
        SetPhase(Phase::kSweepPhase);
        LOG(INFO) << "relocating live objects....";
        if(!Allocator::GetNewHeap()->VisitObjects(this)){
            LOG(ERROR) << "couldn't relocate live objects.";
            return false;
        }

        LOG(INFO) << "notifying references....";
        ReferenceNotifier notifier;
        if(!Allocator::GetNewHeap()->VisitObjects(&notifier)){
            LOG(ERROR) << "couldn't notify weak references.";
            return false;
        }
        if(!HandleBase::VisitHandles(&notifier)){
            LOG(ERROR) << "couldn't notify stack references.";
            return false;
        }
        if(!Server::Accept(&notifier)){
            LOG(ERROR) << "couldn't notify server peer sessions.";
            return false;
        }
        if(!HealthCheckService::Accept(&notifier)){
            LOG(ERROR) << "couldn't notify the health check http sessions.";
            return false;
        }

        LOG(INFO) << "updating references....";
        ReferenceUpdater updater;
        if(!HandleBase::VisitHandles(&updater)){
            LOG(ERROR) << "couldn't update stack references.";
            return false;
        }
        if(!Allocator::GetNewHeap()->VisitObjects(&updater)){
            LOG(ERROR) << "couldn't update weak references.";
            return false;
        }
        if(!Server::Accept(&updater)){
            LOG(ERROR) << "couldn't update server peer sessions.";
            return false;
        }
        if(!HealthCheckService::Accept(&updater)){
            LOG(ERROR) << "couldn't update the health check service http sessions.";
            return false;
        }

        LOG(INFO) << "sweeping objects....";
        ObjectSweeper sweeper(this);
        if(!Allocator::GetNewHeap()->VisitObjects(&sweeper)){
            LOG(ERROR) << "couldn't sweep objects.";
            return false;
        }
        return true;
    }

    bool Scavenger::Visit(Object* obj){
        intptr_t size = obj->GetSize();

        new_stats_.num_processed_++;
        new_stats_.bytes_processed_ += size;
        if(obj->IsMarked()){
            if(obj->IsReadyForPromotion()){
                if(!PromoteObject(obj)){
                    LOG(ERROR) << "couldn't promote object.";
                    return false;
                }
            } else{
                if(!ScavengeObject(obj)){
                    LOG(ERROR) << "couldn't scavenge object.";
                    return false;
                }
            }
        }

        return true;
    }

#ifdef TOKEN_DEBUG
    void Scavenger::PrintNewHeap(){
        LOG(INFO) << "New Heap:";
        ObjectPointerPrinter printer;
        if(!Allocator::GetNewHeap()->VisitObjects(&printer))
            LOG(WARNING) << "couldn't print new heap";
    }

    void Scavenger::PrintOldHeap(){
        LOG(INFO) << "Old Heap:";
        ObjectPointerPrinter printer;
        if(!Allocator::GetOldHeap()->VisitObjects(&printer))
            LOG(WARNING) << "couldn't print old heap";
    }
#endif//TOKEN_DEBUG

    bool Scavenger::Scavenge(bool is_major){
        if(is_major){
            LOG(INFO) << "performing major garbage collection...";
        } else{
            LOG(INFO) << "performing minor garbage collection...";
        }

        //TODO: apply major collection steps to Scavenger::ScavengeMemory()
        Scavenger scavenger(is_major);
#ifdef TOKEN_DEBUG
        scavenger.PrintNewHeap();
        scavenger.PrintOldHeap();
#endif//TOKEN_DEBUG
        {
            // Mark Phase
            LOG(INFO) << "marking objects....";
            if(!scavenger.ProcessRoots()){
                return false;//TODO: handle error
            }
            scavenger.GetTimeline().Push("Mark Objects");
        }

        {
            // Sweep Phase
            LOG(INFO) << "scavenging memory....";
            if(!scavenger.ScavengeMemory()){
                return false;//TODO: handle error
            }
            scavenger.GetTimeline().Push("Scavenge Memory");
        }

        LOG(INFO) << "cleaning....";
        //TODO fix scavenger cleanup routine
        Allocator::GetNewHeap()->GetFromSpace()->Reset();
        Allocator::GetNewHeap()->SwapSpaces();

#ifdef TOKEN_DEBUG
        sleep(1);
        LOG(INFO) << scavenger.GetTimeline();
        LOG(INFO) << scavenger.GetNewStats();
        LOG(INFO) << scavenger.GetOldStats();
        scavenger.PrintNewHeap();
        scavenger.PrintOldHeap();
#endif//TOKEN_DEBUG
        return true;
    }
}