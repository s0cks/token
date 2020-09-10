#include "scavenger.h"

#include "transaction.h"

namespace Token{
    class LiveObjectMarker : public ObjectPointerVisitor{
    public:
        bool Visit(Object* obj){
            if(obj->HasStackReferences()){
                LOG(INFO) << "marking object: [" << std::hex << (uword)obj << "] := " << obj->ToString();
                obj->SetMarkedBit();
            }
            return true;
        }
    };

    class ObjectFinalizer : public ObjectPointerVisitor{
    public:
        bool Visit(Object* obj){
            if(!obj->IsMarked()){
                LOG(INFO) << "finalizing object @" << std::hex << (uword)obj;
                obj->~Object();//TODO: call finalizer
                obj->ptr_ = 0;
            }
            return true;
        }
    };

    class ObjectRelocator : public ObjectPointerVisitor{
    private:
        Semispace* dest_;
    public:
        ObjectRelocator(Semispace* dest):
            ObjectPointerVisitor(),
            dest_(dest){}

        bool Visit(Object* obj){
            if(obj->IsMarked()){
                LOG(INFO) << "relocating object @" << std::hex << (uword)obj;
                size_t size = obj->GetSize();
                void* nptr = dest_->Allocate(size);
                obj->ptr_ = reinterpret_cast<uword>(nptr);
                memcpy((void*)obj->ptr_, (void*)obj, size);
            }
            return true;
        }
    };

    class LiveObjectReferenceUpdater : public ObjectPointerVisitor,
                                              WeakReferenceVisitor{
    public:
        bool Visit(Object** field) const{
            Object* obj = (*field);
            if(obj) (*field) = (Object*)obj->ptr_;
            return true;
        }

        bool Visit(Object* obj){
            if(!obj->IsMarked()) obj->Accept(this);
            return true;
        }
    };

    class RootObjectReferenceUpdater : public WeakObjectPointerVisitor{
    public:
        bool Visit(Object** root){
            Object* obj = (*root);
            if(!obj) {
                return true;
            }
            LOG(INFO) << "visiting root: " << obj->ToString();
            (*root) = (Object*)obj->ptr_;
            return true;
        }
    };

    bool Scavenger::Scavenge(){
        LOG(INFO) << "marking live objects....";
        LiveObjectMarker marker;
        GetFromSpace()->Accept(&marker);

        LOG(INFO) << "finalizing objects....";
        ObjectFinalizer finalizer;
        GetFromSpace()->Accept(&finalizer);

        LOG(INFO) << "relocating live objects....";
        ObjectRelocator relocator(GetToSpace());
        GetFromSpace()->Accept(&relocator);

        //TODO: notify references?

        LOG(INFO) << "updating root references....";
        RootObjectReferenceUpdater root_updater;
        Allocator::VisitRoots(&root_updater);

        LOG(INFO) << "updating references....";
        LiveObjectReferenceUpdater ref_updater;
        GetFromSpace()->Accept(&ref_updater);

        LOG(INFO) << "cleaning....";
        GetFromSpace()->Reset();
        GetHeap()->SwapSpaces();
        return true;
    }
}