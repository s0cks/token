#include "scavenger.h"
#include "heap.h"
#include "allocator.h"

namespace Token{
    class GCObjectPointerMarker : public ObjectPointerVisitor{
    private:
        GCMarker marker_;
    public:
        GCObjectPointerMarker(Color color):
            ObjectPointerVisitor(),
            marker_(color){}
        virtual ~GCObjectPointerMarker(){}

        GCMarker GetMarker() const{
            return marker_;
        }

        virtual bool Visit(RawObject* obj){
            GetMarker().MarkObject(obj);
            return true;
        }
    };

    class GCObjectRootsMarker : public GCObjectPointerMarker{
    public:
        GCObjectRootsMarker(): GCObjectPointerMarker(Color::kBlack){}
        ~GCObjectRootsMarker(){}

        bool Visit(RawObject* obj){
            GetMarker().MarkObject(obj);
            GCObjectPointerMarker refs_marker(Color::kGray);
            return obj->VisitOwnedReferences(&refs_marker);
        }
    };

    bool Scavenger::DarkenRoots(){
        GCObjectRootsMarker marker;
        return Allocator::VisitRoots(&marker);
    }

    class HeapObjectPointerVisitor : public ObjectPointerVisitor{
    private:
        Scavenger* parent_;
    protected:
        RawObject* Evacuate(RawObject* obj){
            return GetParent()->Evacuate(obj);
        }
    public:
        HeapObjectPointerVisitor(Scavenger* parent):
            parent_(parent){}
        virtual ~HeapObjectPointerVisitor() = default;

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
    };

    class ObjectPromoter : public HeapObjectPointerVisitor{
    public:
        ObjectPromoter(Scavenger* parent): HeapObjectPointerVisitor(parent){}
        ~ObjectPromoter(){}

        bool Visit(RawObject* obj){
            if(!GetFromSpace()->Contains(obj->GetObjectAddress())){
                LOG(WARNING) << "from space doesn't contain object: " << (*obj);
                return false;
            }
            if(obj->GetColor() == Color::kWhite) return true;
            if(!obj->IsForwarding()) Evacuate(obj);
            return true;
        }
    };

    Semispace* Scavenger::GetFromSpace() const{
        return GetHeap()->GetFromSpace();
    }

    Semispace* Scavenger::GetToSpace() const{
        return GetHeap()->GetToSpace();
    }

    bool Scavenger::EvacuateObjects(){
        ObjectPromoter promoter(this);
        if(!Allocator::VisitRoots(&promoter)){
            LOG(WARNING) << "couldn't promote roots!";
            return false;
        }

        if(!Allocator::VisitAllocated(&promoter)){
            LOG(WARNING) << "couldn't promote live objects";
            return false;
        }
        return true;
    }

    RawObject* Scavenger::Evacuate(Token::RawObject* obj){
        RawObject* nobj = GetToSpace()->Allocate(obj->GetObjectSize());
        memcpy(nobj->GetObjectPointer(), obj->GetObjectPointer(), obj->GetObjectSize());
        obj->SetForwardingAddress(nobj->GetObjectAddress());
        Allocator::MoveObject(obj, nobj);
        return nobj;
    }

    bool Scavenger::Scavenge(Heap* heap){
        Scavenger scavenger(heap);
        if(!scavenger.DarkenRoots()){
            LOG(WARNING) << "couldn't darken roots!";
            return false;
        }
        
        if(!scavenger.EvacuateObjects()){
            LOG(WARNING) << "couldn't process objects!";
            return false;
        }
        heap->SwapSpaces();
        return true;
    }
}