#include "heap.h"
#include "allocator.h"

#if defined(TOKEN_USE_CHENEYGC)
#include "mc_scavenger.h"
#else
#include "ms_scavenger.h"
#endif //TOKEN_USE_CHENEYGC

namespace Token{
    class ObjectPointerMarker : public ObjectPointerVisitor{
    protected:
        Marker marker_;

        bool MarkObject(RawObject* obj){
            marker_.MarkObject(obj);
            return true;
        }
    public:
        ObjectPointerMarker(Color color):
            ObjectPointerVisitor(),
            marker_(color){}
        ~ObjectPointerMarker(){}

        Marker GetMarker() const{
            return marker_;
        }

        Color GetColor() const{
            return GetMarker().GetColor();
        }

        virtual bool Visit(RawObject* obj){
            if(marker_.GetColor() != obj->GetColor()) return MarkObject(obj); // mark object
            //do nothing
            return true;
        }
    };

    bool Scavenger::DarkenRoots(){
        ObjectPointerMarker marker(Color::kBlack);
        return Allocator::VisitRoots(&marker);
    }

    class ReachableObjectPointerMarker : public ObjectPointerMarker{
    public:
        ReachableObjectPointerMarker(Color color): ObjectPointerMarker(color){}
        ~ReachableObjectPointerMarker(){}

        bool Visit(RawObject* obj){
            if(obj->GetColor() == GetColor()) return true; //skip object, already marked
            if(obj->IsReachable()) return MarkObject(obj);
            // do nothing
            return true;
        }
    };

    bool Scavenger::MarkObjects(){
        ReachableObjectPointerMarker marker(Color::kGray);
        return Allocator::VisitAllocated(&marker);
    }
}