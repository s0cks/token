#include "heap.h"
#include "timeline.h"
#include "scavenger.h"
#include "transaction.h"

namespace Token{
    static inline std::string
    ObjectLocation(Object* obj){
        std::stringstream ss;
        ss << "[" << std::hex << obj << ":" << std::dec << obj->GetSize() << "]";
        return ss.str();
    }

    class Marker : public WeakObjectPointerVisitor{
    private:
        std::vector<uword>& stack_;
    public:
        Marker(std::vector<uword>& stack):
            stack_(stack){}

        bool Visit(Object** obj){
            Object* data = (*obj);
            stack_.push_back((uword)data);
            return true;
        }
    };

    bool Scavenger::ScavengeObject(Object* obj){
        LOG(INFO) << "scavenging object " << ObjectLocation(obj);
        intptr_t size = obj->GetSize();
        void* nptr = Allocator::GetNewHeap()->GetToSpace()->Allocate(size);
        obj->ptr_ = (uword)nptr;
        ((Object*)nptr)->size_ = size;
        obj->IncrementCollectionsCounter();
        LOG(INFO) << "new destination: [" << std::hex << nptr << ":" << std::dec << size << "]";
        return true;
    }

    bool Scavenger::PromoteObject(Object* obj){
        LOG(INFO) << "promoting object " << ObjectLocation(obj);
        intptr_t size = obj->GetSize();
        void* nptr = Allocator::GetOldHeap()->Allocate(size);
        obj->ptr_ = (uword)nptr;
        return true;
    }

    bool Scavenger::FinalizeObject(Object* obj){
        LOG(INFO) << "finalizing object " << ObjectLocation(obj);
        obj->~Object(); //TODO: Call finalizer for obj
        obj->ptr_ = 0;
        return true;
    }

    bool Scavenger::ProcessRoots(){
        SetPhase(Phase::kMarkPhase);

        Marker marker(work_);
        if(!HandleBase::VisitHandles(&marker)){
            LOG(WARNING) << "couldn't visit current handles.";
            return false;
        }

        while(HasWork()){
            Object* obj = (Object*)work_.back();
            work_.pop_back();
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
            if(obj->IsMarked()){
                LOG(INFO) << "relocating " << ObjectLocation(obj) << " => " << ObjectLocation((Object*)obj->ptr_);
                (*field) = (Object*)obj->ptr_;
            }
            return true;
        }
    };

    class StackReferenceNotifier : public WeakObjectPointerVisitor,
                                   public ObjectPointerVisitor{
    public:
        bool Visit(Object* obj){
            return obj->Accept(this);
        }

        bool Visit(Object** field){
            Object* obj = (*field);
            LOG(INFO) << "relocating " << ObjectLocation(obj) << " => " << ObjectLocation((Object*)obj->ptr_);
            (*field) = (Object*)obj->ptr_;
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

    class ReferenceUpdater : public ObjectPointerVisitor{
    public:
        bool Visit(Object* obj){
            if(obj->IsMarked()){
                LOG(INFO) << "updating " << ObjectLocation(obj);
                objcpy((void*)obj->ptr_, (const void*)obj, static_cast<size_t>(obj->GetSize()));
            }
            return true;
        }
    };

    bool Scavenger::ScavengeMemory(){
        SetPhase(Phase::kSweepPhase);
        if(!Allocator::GetNewHeap()->VisitObjects(this)){
            LOG(ERROR) << "couldn't scavenge memory.";
            return false;
        }
        LOG(INFO) << "notifying references....";
        StackReferenceNotifier stack_notifier;
        if(!HandleBase::VisitHandles((WeakObjectPointerVisitor*)&stack_notifier)){
            LOG(ERROR) << "couldn't visit current handles.";
            return false;
        }

        ReferenceNotifier ref_notifier;
        if(!Allocator::GetNewHeap()->VisitObjects(&ref_notifier)){
            LOG(ERROR) << "couldn't visit objects in new heap.";
            return false;
        }

        LOG(INFO) << "updating references....";
        ReferenceUpdater updater;
        if(!Allocator::GetNewHeap()->VisitObjects(&updater)){
            LOG(WARNING) << "couldn't visit current handles.";
            return false;
        }
        return true;
    }

    bool Scavenger::Visit(Object* obj){
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
        } else{
            if(!FinalizeObject(obj)){
                LOG(ERROR) << "couldn't finalize object.";
                return false;
            }
        }

        return true;
    }

    bool Scavenger::Scavenge(bool is_major){
        if(is_major){
            LOG(INFO) << "performing major garbage collection...";
        } else{
            LOG(INFO) << "performing minor garbage collection...";
        }

        //TODO: apply major collection steps to Scavenger::ScavengeMemory()
#if defined(TOKEN_DEBUG)
        Timeline timeline(is_major ? "MajorGC" : "MinorGC");
        timeline.Push("Start");
        ScavengerStats stats(Allocator::GetNewHeap());
#endif//TOKEN_DEBUG

        //TODO: do we need an instance of Scavenger here?
        Scavenger scavenger;
        {
            sleep(1);
            // Mark Phase
            LOG(INFO) << "marking objects....";
            timeline.Push("Mark Objects");
            if(!scavenger.ProcessRoots()){
                return false;//TODO: handle error
            }
        }

        {
            // Sweep Phase
            sleep(1);
            timeline.Push("Scavenge Memory");
            LOG(INFO) << "scavenging memory....";
            if(!scavenger.ScavengeMemory()){
                return false;//TODO: handle error
            }
        }

        LOG(INFO) << "cleaning....";
        //TODO fix scavenger cleanup routine
        Allocator::GetNewHeap()->GetFromSpace()->Reset();
        Allocator::GetNewHeap()->SwapSpaces();

#if defined(TOKEN_DEBUG)
        sleep(1);
        timeline.Push("Stop");


        LOG(INFO) << "Scavenger Stats (New Heap):";
        LOG(INFO) << " - Scavenged (Bytes): " << stats.GetBytesCollected();
        LOG(INFO) << " - Promoted (Bytes): " << stats.GetBytesPromoted();
        LOG(INFO) << " - Timeline (" << timeline.GetTotalTime() << "ms): ";
        size_t idx = 0;
        for(auto& event : timeline){
            LOG(INFO) << "     " << idx << ". " << event.GetName() << ": " << GetTimestampFormattedReadable(event.GetTimestamp());
        }
#endif//TOKEN_DEBUG
        return true;
    }
}