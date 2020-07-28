#include "scavenger.h"

namespace Token{
    class LiveObjectMarker : public ObjectPointerVisitor{
    public:
        bool Visit(RawObject* obj){
            LOG(INFO) << "object: " << obj->ToString() << "(References: " << obj->GetReferenceCount() << "; Color: " << obj->GetColor() << ")";
            if(obj->HasStackReferences()){
                LOG(INFO) << "marking object: " << std::hex << obj;
                obj->SetColor(Color::kMarked);
            }
            return true;
        }
    };

    class ObjectFinalizer : public ObjectPointerVisitor{
    public:
        bool Visit(RawObject* obj){
            if(obj->IsGarbage()){
                LOG(INFO) << "finalizing object: " << std::hex << obj;
                obj->~RawObject();//TODO: call finalizer
                obj->ptr_ = 0;
            }
            return true;
        }
    };

    class ObjectRelocator : public ObjectPointerVisitor{
    private:
        Semispace dest_;
        Heap* promotion_;
    public:
        ObjectRelocator(const Semispace& dest, Heap* promotion):
            ObjectPointerVisitor(),
            dest_(dest),
            promotion_(promotion){}

        bool Visit(RawObject* obj){
            if(obj->IsMarked()){
                size_t size = obj->GetAllocatedSize();
                void* nptr = nullptr;
                if(obj->IsReadyForPromotion()){
                    nptr = promotion_->Allocate(size);
                } else{
                    nptr = dest_.Allocate(size);
                }

                obj->ptr_ = reinterpret_cast<uword>(nptr);
                LOG(INFO) << std::hex << obj << " relocated to: " << std::hex << nptr;
            }
            return true;
        }
    };

    class LiveObjectCopier : public ObjectPointerVisitor{
    public:
        bool Visit(RawObject* obj){
            if(obj->IsMarked()){
                LOG(INFO) << "copying " << obj->ToString() << " to: " << std::hex << obj->ptr_;
                obj->SetColor(Color::kFree);
                memcpy((void*)obj->ptr_, (void*)obj, obj->GetAllocatedSize());
            }
            return true;
        }
    };

    class UpdateIterator : public FieldIterator{
    public:
        void operator()(RawObject** field) const{
            RawObject* obj = (*field);
            if(!obj) return;
            (*field) = (RawObject*)obj->ptr_;
        }
    };

    class LiveObjectReferenceUpdater : public ObjectPointerVisitor{
    public:
        bool Visit(RawObject* obj){
            if(!obj->IsGarbage()) obj->Accept(UpdateIterator{});
            return true;
        }
    };

    class RootObjectReferenceUpdater : public RootObjectPointerVisitor{
    public:
        bool Visit(RawObject** root){
            RawObject* obj = (*root);
            if(!obj) {
                LOG(WARNING) << "skipping null root";
                return true;
            }
            LOG(INFO) << "migrating root " << std::hex << (*root) << " to: " << std::hex << obj->ptr_;
            (*root) = (RawObject*)obj->ptr_;
            return true;
        }
    };

    void Scavenger::ScavengeMemory(){
        LOG(INFO) << "marking live objects....";
        LiveObjectMarker marker;
        GetFromSpace().Accept(&marker);

        LOG(INFO) << "finalizing objects....";
        ObjectFinalizer finalizer;
        GetFromSpace().Accept(&finalizer);

        LOG(INFO) << "relocating live objects....";
        switch(GetHeap()->GetSpace()){
            case Space::kEdenSpace:{
                ObjectRelocator relocator(GetToSpace(), Allocator::GetSurvivorHeap());
                GetFromSpace().Accept(&relocator);
                break;
            }
            case Space::kSurvivorSpace:{
                //TODO:
                //ObjectPromoter promoter(Allocator::GetTenuredHeap());
                //GetFromSpace().Accept(&promoter);
                break;
            }
            case Space::kTenuredSpace: break; // cannot promote any further
            case Space::kStackSpace:
            default:
                LOG(WARNING) << "unknown heap space " << GetHeap()->GetSpace() << " during scavenging";
                return;
        }

        //TODO: notify references?

        LOG(INFO) << "updating root references....";
        RootObjectReferenceUpdater root_updater;
        Allocator::VisitRoots(&root_updater);

        LOG(INFO) << "updating references....";
        LiveObjectReferenceUpdater ref_updater;
        GetFromSpace().Accept(&ref_updater);

        LOG(INFO) << "copying live objects...";
        LiveObjectCopier copier;
        GetFromSpace().Accept(&copier);

        LOG(INFO) << "cleaning....";
        GetFromSpace().Reset();
        GetHeap()->SwapSpaces();
    }
}