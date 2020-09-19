#include "heap.h"
#include "timeline.h"
#include "scavenger.h"
#include "transaction.h"

namespace Token{
    ScavengerStats::ScavengerStats(Heap* heap):
            heap_(heap),
            start_size_(heap->GetAllocatedSize()),
            stop_size_(0){}

    void ScavengerStats::CollectionFinished(){
        stop_size_ = GetHeap()->GetAllocatedSize();
    }

    class Marker : public ObjectPointerVisitor,
                   public WeakObjectPointerVisitor{
    private:
        std::vector<uword>& stack_;
    public:
        Marker(std::vector<uword>& stack):
            ObjectPointerVisitor(),
            stack_(stack){}

        bool Visit(RawObject** ptr){
            return Visit(*ptr);
        }

        bool Visit(RawObject* obj){
            stack_.push_back((uword)obj);
            return true;
        }
    };

    bool Scavenger::ScavengeObject(RawObject* obj){
        LOG(INFO) << "scavenging object [" << std::hex << obj << ":" << std::dec << obj->GetObjectSize() << "]";
        intptr_t size = obj->GetAllocatedSize();
        void* nptr = Allocator::GetNewHeap()->GetToSpace()->Allocate(size);
        obj->ptr_ = (uword)nptr;
        obj->IncrementCollectionsCounter();
        memcpy((void*)obj->ptr_, (void*)obj, size); // is this safe to do???? no
        return true;
    }

    bool Scavenger::PromoteObject(RawObject* obj){
        LOG(INFO) << "promoting object [" << std::hex << obj << ":" << std::dec << obj->GetObjectSize() << "]";
        intptr_t size = obj->GetAllocatedSize();
        void* nptr = Allocator::GetOldHeap()->Allocate(size);
        memcpy(nptr, (void*)obj, size); // is this safe to do????
        obj->ptr_ = (uword)nptr;
        memcpy((void*)obj->ptr_, (void*)obj, size);
        return true;
    }

    bool Scavenger::FinalizeObject(RawObject* obj){
        LOG(INFO) << "finalizing object [" << std::hex << obj << ":" << std::dec << obj->GetObjectSize() << "]";
        obj->~RawObject(); //TODO: Call finalizer for obj
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

            if (!obj->IsMarked()) {
                LOG(INFO) << "marking object [" << std::hex << obj << ":" << std::dec << obj->GetObjectSize() << "]";
                obj->SetMarked();
                if (!obj->Accept(&marker)) {
                    LOG(WARNING) << "couldn't visit object references.";
                    continue;
                }
            }
        }

        return true;
    }

    bool Scavenger::ScavengeMemory(){
        SetPhase(Phase::kSweepPhase);
        if(!Allocator::GetNewHeap()->VisitObjects(this)){
            LOG(ERROR) << "couldn't scavenge memory.";
            return false;
        }

        return true;
    }

    bool Scavenger::Visit(RawObject* obj){
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

    class ReferenceNotifier :
            public ObjectPointerVisitor,
            public WeakObjectPointerVisitor{
    public:
        bool Visit(RawObject* obj){
            return obj->Accept(this);
        }

        bool Visit(RawObject** field){
            RawObject* obj = (*field);
            if(obj->IsMarked()){
                (*field) = (RawObject*)obj->ptr_;
            }
            return true;
        }
    };

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

        //TODO: notify references?
        LOG(INFO) << "updating references....";
        ReferenceNotifier ref_updater;
        if(!HandleBase::VisitHandles(&ref_updater)){
            LOG(ERROR) << "couldn't visit handles.";
            return false;
        }

        if(!Allocator::GetNewHeap()->VisitObjects(&ref_updater)){
            LOG(ERROR) << "couldn't visit new heap.";
            return false;
        }

        LOG(INFO) << "cleaning....";
        //TODO fix scavenger cleanup routine
        Allocator::GetNewHeap()->GetFromSpace()->Reset();
        Allocator::GetNewHeap()->SwapSpaces();

#if defined(TOKEN_DEBUG)
        sleep(1);
        timeline.Push("Stop");
        stats.CollectionFinished();
        LOG(INFO) << "Scavenger Stats (New Heap):";
        LOG(INFO) << "  - Bytes Collected: " << stats.GetTotalBytesCollected();
        LOG(INFO) << "Timeline (New Heap):";
        if(!timeline.Print()) LOG(WARNING) << "couldn't print timeline";
#endif//TOKEN_DEBUG
        return true;
    }
}