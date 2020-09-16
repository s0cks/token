#include "heap.h"
#include "scavenger.h"
#include "transaction.h"

namespace Token{
    class LiveObjectMarker : public ObjectPointerVisitor{
    public:
        bool Visit(RawObject* obj){
            if(obj->HasStackReferences()){
                LOG(INFO) << "marking object: " << obj->ToString();
                obj->SetMarked();
            }
            return true;
        }
    };

    class ObjectFinalizer : public ObjectPointerVisitor{
    public:
        bool Visit(RawObject* obj){
            if(!obj->IsMarked()){
                obj->~RawObject();//TODO: call finalizer
                obj->ptr_ = 0;
            }
            return true;
        }
    };

    class ObjectRelocator : public ObjectPointerVisitor{
    public:
        bool Visit(RawObject* obj){
            if(obj->IsMarked()){
                LOG(INFO) << "relocating object: " << obj->ToString();
                size_t size = obj->GetAllocatedSize();
                void* nptr = nullptr;
                if(obj->IsReadyForPromotion()){
                    //TODO: properly re-allocate object in old heap
                    nptr = Allocator::GetOldHeap()->Allocate(size);
                } else{
                    //TODO: properly re-allocate object in to space
                    nptr = Allocator::GetNewHeap()->GetToSpace().Allocate(size);
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
                obj->ClearMarked();
                memcpy((void*)obj->ptr_, (void*)obj, obj->GetAllocatedSize());
            }
            return true;
        }
    };

    class LiveObjectReferenceUpdater : public ObjectPointerVisitor,
                                              WeakReferenceVisitor{
    public:
        bool Visit(RawObject** field) const{
            RawObject* obj = (*field);
            if(obj) (*field) = (RawObject*)obj->ptr_;
            return true;
        }

        bool Visit(RawObject* obj){
            if(obj->IsMarked()) obj->Accept(this);
            return true;
        }
    };

    class RootObjectReferenceUpdater : public WeakObjectPointerVisitor{
    public:
        bool Visit(RawObject** root){
            RawObject* obj = (*root);
            if(!obj) {
                return true;
            }
            LOG(INFO) << "visiting root: " << obj->ToString();
            (*root) = (RawObject*)obj->ptr_;
            return true;
        }
    };

    bool Scavenger::ScavengeMemory(){
        //TODO: apply major collection steps to Scavenger::ScavengeMemory()
        LOG(INFO) << "marking live objects....";
        LiveObjectMarker marker;
        if(!Allocator::GetNewHeap()->Accept(&marker)){
            LOG(ERROR) << "couldn't visit new heap.";
            return false;
        }

        LOG(INFO) << "finalizing objects....";
        ObjectFinalizer finalizer;
        if(!Allocator::GetNewHeap()->Accept(&finalizer)){
            LOG(ERROR) << "couldn't visit new heap.";
            return false;
        }

        LOG(INFO) << "relocating live objects....";
        ObjectRelocator relocator;
        if(!Allocator::GetNewHeap()->Accept(&relocator)){
            LOG(ERROR) << "couldn't visit new heap.";
            return false;
        }

        //TODO: notify references?
        LOG(INFO) << "updating root references....";
        RootObjectReferenceUpdater root_updater;
        Allocator::VisitRoots(&root_updater);

        LOG(INFO) << "updating references....";
        LiveObjectReferenceUpdater ref_updater;
        if(!Allocator::GetNewHeap()->Accept(&ref_updater)){
            LOG(ERROR) << "couldn't visit new heap.";
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
        Allocator::GetNewHeap()->GetFromSpace().Reset();
        Allocator::GetNewHeap()->SwapSpaces();
    }
}