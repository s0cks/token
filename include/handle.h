#ifndef TOKEN_HANDLE_H
#define TOKEN_HANDLE_H

namespace Token{
    class Object;
    class HandleBase{
    private:
        Object** ptr_;
    protected:
        HandleBase();
        HandleBase(Object* obj);
        HandleBase(const HandleBase& h);
        ~HandleBase();

        Object* GetPointer() const{
            return ptr_ ? (*ptr_) : nullptr;
        }

        void operator=(Object* obj);
        void operator=(const HandleBase& h);
    };

    template<typename T>
    class Handle : HandleBase{
    public:
        Handle(): HandleBase(){}
        Handle(T* obj): HandleBase((Object*)obj){}
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

        template<class S> friend class Handle;
    };
}

#endif //TOKEN_HANDLE_H