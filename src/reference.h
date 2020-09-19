#ifndef TOKEN_REFERENCE_H
#define TOKEN_REFERENCE_H

#include "common.h"

namespace Token{
    class RawObject;
    class WeakObjectPointerVisitor;
    class Reference{
    protected:
        uword owner_;
        uword target_;
        RawObject** slot_;

        Reference(uword src, uword dst, RawObject** slot):
            owner_(src),
            target_(dst),
            slot_(slot){}
    public:
        virtual ~Reference() = default;

        uword GetOwnerAddress() const{
            return owner_;
        }

        RawObject* GetOwner() const{
            return (RawObject*)owner_;
        }

        uword GetTargetAddress() const{
            return target_;
        }

        RawObject* GetTarget() const{
            return (RawObject*)target_;
        }

        void operator=(const Reference& ref){
            owner_ = ref.owner_;
            target_ = ref.target_;
            slot_ = ref.slot_;
        }

        friend bool operator==(const Reference& a, const Reference& b){
            return a.GetOwner() == b.GetOwner()
                && a.GetTarget() == b.GetTarget() // is this the most efficient/suitable operator== for Reference?
                && memcmp(a.slot_, b.slot_, sizeof(RawObject**)) == 0;
        }

        friend bool operator!=(const Reference& a, const Reference& b){
            return !operator==(a, b);
        }

        bool Accept(WeakObjectPointerVisitor* vis);
        virtual bool IsWeak() const{ return false; }
        virtual bool IsStrong() const{ return false; }
    };

    class WeakReference : public Reference{
    public:
        WeakReference(uword src, uword dst, RawObject** slot):
            Reference(src, dst, slot){}
        WeakReference(RawObject* src, RawObject* dst, RawObject** slot):
            Reference((uword)src, (uword)dst, slot){}
        ~WeakReference() = default;

        bool IsWeak() const{
            return true;
        }

        void operator=(const WeakReference& ref){
            Reference::operator=(ref);
        }
    };
}

#endif //TOKEN_REFERENCE_H