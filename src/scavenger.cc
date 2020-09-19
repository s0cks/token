#include "heap.h"
#include "scavenger.h"
#include "transaction.h"

namespace Token{
    ScavengerStats::ScavengerStats(Scavenger* parent, Heap* heap):
        parent_(parent),
        heap_(heap),
        start_time_(GetCurrentTimestamp()),
        stop_time_(0),
        start_size_(heap->GetAllocatedSize()),
        stop_size_(0){}

    void ScavengerStats::CollectionFinished(){
        stop_time_ = GetCurrentTimestamp();
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

    class ObjectRelocator : public ObjectPointerVisitor{
    public:
        bool Visit(RawObject* obj){
            if(!obj->IsMarked()){
                LOG(INFO) << "finalizing object [" << std::hex << obj << ":" << std::dec << obj->GetObjectSize() << "]";
                obj->~RawObject(); //TODO: Call finalizer for obj
                obj->ptr_ = 0;
            } else{
                intptr_t size = obj->GetAllocatedSize();
                void* nptr = nullptr;
                if(obj->IsReadyForPromotion()){
                    LOG(INFO) << "promoting object [" << std::hex << obj << ":" << std::dec << obj->GetObjectSize() << "]";
                    //TODO: properly re-allocate object in old heap
                    nptr = Allocator::GetOldHeap()->Allocate(size);
                } else{
                    LOG(INFO) << "scavenging object [" << std::hex << obj << ":" << std::dec << obj->GetObjectSize() << "]";
                    //TODO: properly re-allocate object in to space
                    nptr = Allocator::GetNewHeap()->GetToSpace()->Allocate(size);
                }
                obj->ptr_ = reinterpret_cast<uword>(nptr);
            }
            return true;
        }
    };

    class LiveObjectCopier : public ObjectPointerVisitor{
    public:
        bool Visit(RawObject* obj){
            if(obj->IsMarked()){
                obj->IncrementCollectionsCounter();
                obj->ClearMarked();
                memcpy((void*)obj->ptr_, (void*)obj, obj->GetAllocatedSize());
            }
            return true;
        }
    };

    class LiveObjectReferenceUpdater : public ObjectPointerVisitor,
                                       public WeakObjectPointerVisitor{
    public:
        bool Visit(RawObject** field){
            RawObject* obj = (*field);
            if(obj->IsMarked()){
                (*field) = (RawObject*)obj->ptr_;
            }
            return true;
        }

        bool Visit(RawObject* obj){
            if(obj->IsMarked()) obj->Accept(this);
            return true;
        }
    };

    bool Scavenger::ScavengeMemory(){
        //TODO: apply major collection steps to Scavenger::ScavengeMemory()
#if defined(TOKEN_DEBUG)
        ScavengerStats new_heap_stats(this, Allocator::GetNewHeap());
        LOG(INFO) << "starting garbage collection @" << GetTimestampFormattedReadable(new_heap_stats.GetStartTime());
#endif//TOKEN_DEBUG

        std::vector<uword> stack;
        Marker marker(stack);
        if(!HandleBase::VisitHandles(&marker)){
            LOG(WARNING) << "couldn't visit current handles.";
            return false;
        }

        LOG(INFO) << "marking live objects....";
        while(!stack.empty()){
            Object* obj = (Object*) stack.back();
            stack.pop_back();

            if (!obj->IsMarked()) {
                LOG(INFO) << "marking object [" << std::hex << obj << ":" << std::dec << obj->GetObjectSize() << "]";
                obj->SetMarked();
                if (!obj->Accept(&marker)) {
                    LOG(WARNING) << "couldn't visit object references.";
                    continue;
                }
            }
        }

        LOG(INFO) << "scavenging memory....";
        ObjectRelocator relocator;
        if(!Allocator::GetNewHeap()->Accept(&relocator)){
            LOG(ERROR) << "couldn't visit new heap.";
            return false;
        }

        //TODO: notify references?
        LOG(INFO) << "updating references....";
        LiveObjectReferenceUpdater ref_updater;
        if(!Allocator::GetNewHeap()->Accept(&ref_updater)){
            LOG(ERROR) << "couldn't visit new heap.";
            return false;
        }

        if(!HandleBase::VisitHandles(&ref_updater)){
            LOG(ERROR) << "couldn't visit handles.";
            return false;
        }

        LOG(INFO) << "copying live objects...";
        LiveObjectCopier copier;
        if(!Allocator::GetNewHeap()->Accept(&copier)){
            LOG(ERROR) << "couldn't visit new heap.";
            return false;
        }

        LOG(INFO) << "cleaning....";
        //TODO fix scavenger cleanup routine
        Allocator::GetNewHeap()->GetFromSpace()->Reset();
        Allocator::GetNewHeap()->SwapSpaces();

#if defined(TOKEN_DEBUG)
        new_heap_stats.CollectionFinished();
        LOG(INFO) << "Scavenger Stats (New Heap):";
        LOG(INFO) << "  - Collection Time (Milliseconds): " << new_heap_stats.GetTotalCollectionTimeMilliseconds();
        LOG(INFO) << "  - Total Bytes Collected: " << new_heap_stats.GetTotalBytesCollected();
#endif//TOKEN_DEBUG
        return true;
    }
}