#include "raw_object.h"
#include "object.h"
#include "allocator.h"
#include "alloc/reference.h"

namespace Token{
    bool RawObject::IsReachable() const {
        if(Allocator::IsRoot(GetObjectPointer())) return true;
        for(auto& it : pointing_){
            RawObject* owner = it->GetOwner();
            if(owner->IsReachable()) return true;
        }
        return false;
    }

    bool RawObject::VisitOwnedReferences(ObjectPointerVisitor* vis) const{
        for(auto& it : owned_){
            RawObject* obj = it->GetTarget();
            if(!vis->Visit(obj)) return false;
        }
        return true;
    }

    bool RawObject::VisitPointingReferences(ObjectPointerVisitor* vis) const{
        for(auto& it : pointing_){
            RawObject* obj = it->GetOwner();
            if(!vis->Visit(obj)) return false;
        }
        return true;
    }

    std::string RawObject::ToString() const{
        return GetObject()->ToString();
    }
}
