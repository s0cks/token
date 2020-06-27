#if defined(TOKEN_USE_CHENEYGC)
#include "mc_scavenger.h"
#include "allocator.h"

namespace Token{
    class MarkCopyMemoryScavenger : public ObjectPointerVisitor{
    private:
        MarkCopyScavenger* parent_;

        bool EvacuateObject(RawObject* obj){
            return parent_->Evacuate(obj);
        }
    public:
        MarkCopyMemoryScavenger(MarkCopyScavenger* parent):
                parent_(parent){}
        ~MarkCopyMemoryScavenger(){}

        MarkCopyScavenger* GetParent() const{
            return parent_;
        }

        Heap* GetHeap() const{
            return GetParent()->GetHeap();
        }

        Semispace* GetFromSpace() const{
            return GetHeap()->GetFromSpace();
        }

        Semispace* GetToSpace() const{
            return GetHeap()->GetToSpace();
        }

        bool Visit(RawObject* obj){
            if(!GetFromSpace()->Contains(obj->GetObjectAddress())) return true; // skip
            if(obj->IsGarbage()) return true; // object is garbage don't allow it to persist the scavenge
            if(obj->IsForwarding()) return true; // object is already evacuated
            return EvacuateObject(obj);
        }
    };

    bool MarkCopyScavenger::ScavengeMemory(){
        LOG(INFO) << "running garbage collection process";

        if(!DarkenRoots()){
            LOG(WARNING) << "couldn't darken roots!";
            return false;
        }

        if(!MarkObjects()){
            LOG(WARNING) << "couldn't mark references!";
            return false;
        }

        MarkCopyMemoryScavenger scavenger(this);
        if(!Allocator::VisitAllocated(&scavenger)){
            LOG(WARNING) << "couldn't scavenge memory";
            return false;
        }

        GetHeap()->SwapSpaces();
        LOG(INFO) << "garbage collection complete!";
        return true;
    }

    RawObject* MarkCopyScavenger::Evacuate(Token::RawObject* obj){
        RawObject* nobj = GetToSpace()->Allocate(obj->GetObjectSize());
        memcpy(nobj->GetObjectPointer(), obj->GetObjectPointer(), obj->GetObjectSize());
        obj->SetForwardingAddress(nobj->GetObjectAddress());
        Allocator::MoveObject(obj, nobj);
        return nobj;
    }
}

#endif//TOKEN_USE_CHENEYGC