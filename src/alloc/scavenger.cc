#include "scavenger.h"
#include "heap.h"
#include "allocator.h"

namespace Token{
    class RootReferenceMarker : public ObjectPointerVisitor{
    private:
        Marker marker_;
    public:
        RootReferenceMarker(Color color):
                ObjectPointerVisitor(),
                marker_(color){}
        ~RootReferenceMarker(){}

        bool Visit(RawObject* obj){
            return obj->VisitPointingReferences(&marker_);
        }
    };

    bool Scavenger::Scavenge(Heap* heap){
        MarkCopyScavenger scavenger(heap);
        return scavenger.ScavengeMemory();
    }

    bool Scavenger::DarkenRoots(){
        Marker marker(Color::kBlack);
        return Allocator::VisitRoots(&marker);
    }

    bool Scavenger::MarkObjects(){
        RootReferenceMarker marker(Color::kGray);
        return Allocator::VisitRoots(&marker);
    }

    Semispace* MarkCopyScavenger::GetFromSpace() const{
        return GetHeap()->GetFromSpace();
    }

    Semispace* MarkCopyScavenger::GetToSpace() const{
        return GetHeap()->GetToSpace();
    }

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

        Scavenger* GetParent() const{
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