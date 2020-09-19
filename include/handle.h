#ifndef TOKEN_HANDLE_H
#define TOKEN_HANDLE_H

#include <glog/logging.h>

namespace Token{
    class RawObject;
    class WeakObjectPointerVisitor;
    class HandleBase{ //TODO: remove
        friend class Scavenger;
    private:

        RawObject** ptr_;

        static bool VisitHandles(WeakObjectPointerVisitor* vis);
    protected:
        HandleBase();
        HandleBase(RawObject* obj);
        HandleBase(const HandleBase& h);
        ~HandleBase();

        RawObject* GetPointer() const{
            return ptr_ ? (*ptr_) : nullptr;
        }

        bool IsNull() const{
            return ptr_ == nullptr;
        }

        void operator=(RawObject* obj);
        void operator=(const HandleBase& h);
    };

    template<typename T>
    class Handle : HandleBase{
    public:
        Handle(): HandleBase(){}
        Handle(T* obj): HandleBase((RawObject*)obj){}
        Handle(const Handle& h): HandleBase(h){}

        void operator=(T* obj){
            HandleBase::operator=(obj);
        }

        void operator=(const Handle& h){
            HandleBase::operator=(h);
        }

        T* operator->() const{
            return static_cast<T*>(GetPointer());
        }

        operator T*() const{
            return reinterpret_cast<T*>(GetPointer());
        }

        template<typename U>
        Handle<U> CastTo() const{
            return static_cast<U*>(operator T*());
        }

        template<typename U>
        Handle<U> DynamicCastTo() const {
            return dynamic_cast<U*>(operator T*());
        }

        bool IsNull() const{
            return HandleBase::IsNull();
        }

        template<class S> friend class Handle;

        friend bool operator==(const Handle<T>& a, const Handle<T>& b){
            return a->Equals(b);
        }

        friend bool operator<(const Handle<T>& a, const Handle<T>& b){
            return a->Compare(b);
        }

        friend std::ostream& operator<<(std::ostream& stream, const Handle<T>& handle){
            stream << handle.operator->()->ToString();
            return stream;
        }
    };
}

#endif //TOKEN_HANDLE_H