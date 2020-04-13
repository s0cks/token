#ifndef TOKEN_OBJECT_H
#define TOKEN_OBJECT_H

#include <vector>
#include <string>
#include "common.h"

namespace Token{
#define FOR_EACH_TYPE(V) \
    V(Object) \
    V(Class) \
    V(Field) \
    V(Block) \
    V(Transaction) \
    V(UnclaimedTransaction) \
    V(Number) \
    V(Hash)

#define FORWARD_DECLARE(Name) class Name;
    FOR_EACH_TYPE(FORWARD_DECLARE);
#undef FORWARD_DECLARE

    const intptr_t kOffsetOfPtr = 32;

#define OFFSET_OF(Type, Field) \
    (reinterpret_cast<intptr_t>(&(reinterpret_cast<Type*>(kOffsetOfPtr)->Field)) - kOffsetOfPtr)

    class Instance;

    class Object{
    public:
        virtual ~Object(){}

        virtual std::string ToString(){
            return "Object()";
        }
    };

    class Class : public Object{
    private:
        std::string name_;
        Class* super_;
        std::vector<Field*> fields_;
    public:
        Class(std::string name, Class* super = Class::CLASS_OBJECT):
            name_(name),
            super_(super),
            fields_(),
            Object(){
        }
        ~Class(){}

        std::string GetName(){
            return name_;
        }

        Class* GetParentClass(){
            return super_;
        }

        size_t GetFieldCount(){
            return fields_.size();
        }

        intptr_t GetAllocationSize();
        Field* DefineField(std::string name, Class* type);
        Field* GetField(std::string name);

        Field* GetFieldAt(size_t idx){
            return fields_[idx];
        }

        static Class* CLASS_OBJECT;
        static Class* CLASS_CLASS;
        static Class* CLASS_FIELD;
        static Class* CLASS_BLOCK;
        static Class* CLASS_TRANSACTION;
        static Class* CLASS_UNCLAIMED_TRANSACTION;
        static Class* CLASS_NUMBER;
        static Class* CLASS_HASH;
    };

    class Field : public Object{
    private:
        std::string name_;
        Class* type_;
        Class* owner_;
        intptr_t offset_;
    public:
        Field(std::string& name, Class* owner, Class* type):
            name_(name),
            type_(type),
            offset_(0),
            owner_(owner){
        }
        ~Field(){}

        std::string GetName(){
            return name_;
        }

        Class* GetType(){
            return type_;
        }

        Class* GetOwner(){
            return owner_;
        }

        intptr_t GetOffset(){
            return offset_;
        }
    };

    class Instance : public Object{
    private:
        Class* type_;

        Instance** FieldAddrAtOffset(intptr_t offset){
            return reinterpret_cast<Instance**>(reinterpret_cast<uintptr_t>(this) + offset);
        }

        Instance** FieldAddr(Field* field){
            return FieldAddrAtOffset(field->GetOffset());
        }
    protected:
        Instance(Class* type):
            type_(type){}
    public:
        virtual ~Instance(){}

        Class* GetType(){
            return type_;
        }

        void SetField(Field* field, Instance* value){
            (*FieldAddr(field)) = value;
        }

        Instance* GetField(Field* field){
            return *FieldAddr(field);
        }

        static Instance* New(Class* type);

        static intptr_t TypeOffset(){
            return OFFSET_OF(Instance, type_);
        }
    };

    class Hash : public Instance{
    private:
        std::string value_;
    public:
        Hash(std::string value):
            value_(value),
            Instance(Class::CLASS_HASH){

        }
        ~Hash(){

        }

        std::string GetHash(){
            return value_;
        }

        static intptr_t ValueOffset(){
            return OFFSET_OF(Hash, value_);
        }
    };
}

#endif //TOKEN_OBJECT_H