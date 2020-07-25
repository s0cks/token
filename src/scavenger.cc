#include "scavenger.h"

namespace Token{
    class LiveObjectMarker : public ObjectPointerVisitor{
    public:
        bool Visit(Object* obj){
            if(obj->HasStackReferences()){
                LOG(INFO) << "marking object: " << std::hex << obj;
                obj->SetColor(Object::kBlack);
            }
            return true;
        }
    };

    class ObjectFinalizer : public ObjectPointerVisitor{
    public:
        bool Visit(Object* obj){
            if(obj->IsGarbage()){
                LOG(INFO) << "finalizing object: " << std::hex << obj;
                obj->~Object();//TODO: call finalizer
                obj->ptr_ = nullptr;
            }
            return true;
        }
    };

    class ObjectRelocator : public ObjectPointerVisitor{
    private:
        Semispace dest_;
    public:
        ObjectRelocator(const Semispace& dest):
            ObjectPointerVisitor(),
            dest_(dest){}

        bool Visit(Object* obj){
            if(!obj->IsGarbage() && !obj->IsReadyForPromotion()){
                size_t size = obj->GetSize();
                void* nptr = dest_.Allocate(size);
                obj->ptr_ = static_cast<Object*>(nptr);

                LOG(INFO) << std::hex << obj << " relocated to: " << std::hex << nptr;
            }
            return true;
        }
    };

    class ObjectPromoter : public ObjectPointerVisitor{
    private:
        Heap* dest_;
    public:
        ObjectPromoter(Heap* dest):
            ObjectPointerVisitor(),
            dest_(dest){}

        bool Visit(Object* obj){
            if(!obj->IsGarbage() && obj->IsReadyForPromotion()){
                size_t size = obj->GetSize();
                void* nptr = dest_->Allocate(size);
                obj->ptr_ = static_cast<Object*>(nptr);
                LOG(INFO) << std::hex << obj << " promoted to: " << std::hex << nptr;
            }
            return true;
        }
    };

    class LiveObjectCopier : public ObjectPointerVisitor{
    public:
        bool Visit(Object* obj){
            if(!obj->IsGarbage()){
                LOG(INFO) << "copying " << std::hex << obj << " to: " << std::hex << obj->ptr_;
                obj->SetColor(Object::kFree);
                memcpy((void*)obj->ptr_, (void*)obj, obj->GetSize());
            }
            return true;
        }
    };

    class UpdateIterator : public FieldIterator{
    public:
        void operator()(Object** field) const{
            Object* obj = (*field);
            if(!obj) return;
            (*field) = obj->ptr_;
        }
    };

    class LiveObjectReferenceUpdater : public ObjectPointerVisitor{
    public:
        bool Visit(Object* obj){
            if(!obj->IsGarbage()) obj->Accept(UpdateIterator{});
            return true;
        }
    };

    class RootObjectReferenceUpdater : public RootObjectPointerVisitor{
    public:
        bool Visit(Object** root){
            Object* obj = (*root);
            if(!obj) {
                LOG(WARNING) << "skipping null root";
                return true;
            }
            LOG(INFO) << "migrating root " << std::hex << (*root) << " to: " << std::hex << obj->ptr_;
            (*root) = obj->ptr_;
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
        ObjectRelocator relocator(GetToSpace());
        GetFromSpace().Accept(&relocator);

        LOG(INFO) << "promoting live objects....";
        switch(GetHeap()->GetSpace()){
            case Space::kEdenSpace:{
                ObjectPromoter promoter(Allocator::GetSurvivorHeap());
                GetFromSpace().Accept(&promoter);
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