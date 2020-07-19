#ifndef TOKEN_ARRAY_H
#define TOKEN_ARRAY_H

#include "object.h"

namespace Token{
    class ArrayBase : public Object{
    private:
        size_t length_;
        Object* slots_[1];
    protected:
        static void* operator new(size_t size) = delete;
        static void* operator new(size_t size, size_t length, bool);
        static void operator delete(void*, size_t, bool);
        using Object::operator delete;

        ArrayBase(size_t length);

        void Put(size_t index, Object* obj){
            WriteBarrier(&slots_[index], obj);
        }

        Object* Get(size_t index){
            return slots_[index];
        }

        size_t Length(){
            return length_;
        }

        void Accept(const FieldIterator& it);
    };

    template<typename T>
    class Array : public ArrayBase{
    private:
        Array(size_t size): ArrayBase(size){}

        struct Iterator{
            Handle<Array<T>> self;
            size_t ptr;

            Handle<T> operator*() {
                return self->Get(ptr);
            }

            bool operator !=(const Iterator& another) {
                return ptr != another.ptr;
            }

            Iterator& operator++() {
                ptr++;
                return *this;
            }
        };

        struct Iterable {
            Handle<Array<T>> self;

            Iterator begin() {
                return{ self, 0 };
            }

            Iterator end() {
                return{ self, self->Length() };
            }
        };
    public:
        void Put(size_t index, const Handle<T>& obj){
            ArrayBase::Put(index, obj);
        }

        Handle<T> Get(size_t index){
            return static_cast<T*>(ArrayBase::Get(index));
        }

        size_t Length(){
            return ArrayBase::Length();
        }

        Iterable GetIterable(){
            return { this };
        }

        static Handle<Array> New(size_t length){
            return new (length, false)Array(length);
        }
    };
}

#endif //TOKEN_ARRAY_H