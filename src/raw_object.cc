#include "alloc/reference.h"
#include "raw_object.h"
#include "object.h"

namespace Token{
    bool RawObject::VisitOwnedReferences(ObjectPointerVisitor* vis){
        for(auto& it : owned_){
            RawObject* obj = it->GetTarget();
            if(!vis->Visit(obj)) return false;
        }
        return true;
    }

    bool RawObject::VisitPointingReferences(ObjectPointerVisitor* vis){
        for(auto& it : pointing_){
            RawObject* obj = it->GetOwner();
            if(!vis->Visit(obj)) return false;
        }
        return true;
    }

    std::string RawObject::ToString() const{
        return GetObject()->ToString();
    }

    RawObject::RawObject(void* ptr, size_t size):
            owned_(),
            pointing_(),
            header_(0),
            ptr_(ptr),
            forwarding_address_(0){
        SetForwarding(false);
        SetCondemned(false);
        SetColor(Color::kWhite);
        SetSize(size);
    }
}
