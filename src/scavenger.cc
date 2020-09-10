#include "scavenger.h"

#include "transaction.h"

namespace Token{
    class ObjectFinalizer : public ObjectPointerVisitor{
    public:
        bool Visit(Object* obj){
            if(!obj->IsMarked()){
                LOG(INFO) << "finalizing object [" << std::hex << obj << "] := " << obj->ToString();
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
            LOG(INFO) << "relocating object [" << std::hex << obj << "] := " << obj->ToString();
            size_t size = obj->GetSize();
            void* nptr = dest_->Allocate(size);
            obj->ptr_ = reinterpret_cast<uword>(nptr);
            memcpy((void*)obj->ptr_, (void*)obj, size);
            return true;
        }
    };

    class ReferenceNotifier : public ObjectPointerVisitor,
                              public WeakReferenceVisitor,
                              public WeakObjectPointerVisitor{
    public:
        bool Visit(Object** field) const{
            Object* obj = (*field);
            LOG(INFO) << "visiting root: " << obj->ToString();
            if(obj) (*field) = (Object*)obj->ptr_;
            return true;
        }

        bool Visit(Object* obj){
            obj->Accept(this);
            return true;
        }

        bool Visit(Object** root){
            Object* obj = (*root);
            LOG(INFO) << "visiting root: " << obj->ToString();
            (*root) = (Object*)obj->ptr_;
            return true;
        }
    };

    class ScavengerMarker : public ObjectPointerVisitor,
                            public WeakObjectPointerVisitor,
                            public WeakReferenceVisitor{
    private:
        std::vector<uword>& stack_;
    public:
        ScavengerMarker(std::vector<uword>& stack):
            ObjectPointerVisitor(),
            stack_(stack){}
        ~ScavengerMarker() = default;

        bool Visit(Object** ptr) const{
            if((*ptr)) stack_.push_back((uword)(*ptr));
            return true;
        }

        bool Visit(Object** ptr){
            if((*ptr)) stack_.push_back((uword)(*ptr));
            return true;
        }

        bool Visit(Object* object){
            stack_.push_back((uword)object);
            return true;
        }
    };

    bool Scavenger::Scavenge(){
        LOG(INFO) << "marking objects....";
        std::vector<uword> stack;
        ScavengerMarker marker(stack);

        if(!Allocator::VisitRoots(&marker)){
            LOG(ERROR) << "couldn't mark roots.";
            return false;
        }

        while(!stack.empty()){
            uword address = stack.back();
            stack.pop_back();

            Object* obj = (Object*)address;

            if(!obj->IsMarked()){
                LOG(INFO) << "marking object [" << std::hex << obj << "] := " << obj->ToString();
                obj->SetMarkedBit();
                LOG(INFO) << std::hex << obj << " marked?: " << (obj->IsMarked() ? 'y' : 'n');
                if(!obj->Accept(&marker)){
                    LOG(ERROR) << "couldn't get references for object @" << std::hex << obj->GetStartAddress();
                    return false;
                }
            }
        }

        LOG(INFO) << "relocating marked objects....";
        ObjectRelocator relocator(GetToSpace());
        if(!GetFromSpace()->VisitMarkedObjects(&relocator)){
            LOG(ERROR) << "couldn't relocate marked objects.";
            return false;
        }

        LOG(INFO) << "notifying references....";
        ReferenceNotifier notifier;
        if(!GetFromSpace()->VisitMarkedObjects(&notifier)){
            LOG(ERROR) << "couldn't notify references.";
            return false;
        }

        LOG(INFO) << "finalizing objects....";
        ObjectFinalizer finalizer;
        GetFromSpace()->Accept(&finalizer);

        LOG(INFO) << "cleaning....";
        GetFromSpace()->Reset();
        GetHeap()->SwapSpaces();
        return true;
    }
}