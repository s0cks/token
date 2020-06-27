#ifndef TOKEN_REFERENCE_H
#define TOKEN_REFERENCE_H

#include "raw_object.h"

namespace Token{
    class Reference{
    private:
        RawObject* owner_;
        RawObject* target_;
        void** ptr_;
    public:
        Reference(RawObject* owner, RawObject* target, void** ptr):
                owner_(owner),
                target_(target),
                ptr_(ptr){}
        ~Reference(){}

        void** GetLocation() const{
            return ptr_;
        }

        RawObject* GetOwner() const{
            return owner_;
        }

        RawObject* GetTarget() const{
            return target_;
        }

#define DECLARE_TYPE_CAST(Name) \
    virtual Name##Reference* As##Name##Reference() { return nullptr; } \
    bool Is##Name##Reference() { return As##Name##Reference() != nullptr; }
        FOR_EACH_REFERENCE_TYPE(DECLARE_TYPE_CAST);
#undef DECLARE_TYPE_CAST
    };

#define DEFINE_REFERENCE_TYPE(Name) \
    public: \
        Name##Reference* As##Name##Reference() { return this; }

    class WeakReference : public Reference{
    public:
        WeakReference(RawObject* owner, RawObject* target, void** ptr): Reference(owner, target, ptr){}
        ~WeakReference(){}

    DEFINE_REFERENCE_TYPE(Weak);
    };

    class StrongReference : public Reference{
    public:
        StrongReference(RawObject* owner, RawObject* target, void** ptr):
                Reference(owner, target, ptr){}
        ~StrongReference(){}

    DEFINE_REFERENCE_TYPE(Strong);
    };
}

#endif //TOKEN_REFERENCE_H
