#include "object.h"

namespace Token{
    Class* Class::CLASS_OBJECT = new Class("Object", nullptr);
    Class* Class::CLASS_CLASS = new Class("Class");
    Class* Class::CLASS_FIELD = new Class("Field");
    Class* Class::CLASS_BLOCK = new Class("Block");
    Class* Class::CLASS_TRANSACTION = new Class("Transaction");
    Class* Class::CLASS_UNCLAIMED_TRANSACTION = new Class("UnclaimedTransaction");
    Class* Class::CLASS_NUMBER = new Class("Number");
    Class* Class::CLASS_HASH = new Class("Hash");

    intptr_t Class::GetAllocationSize(){
        intptr_t offset = sizeof(Instance);
        for(auto& it : fields_){
            offset += sizeof(intptr_t);
        }
        return offset;
    }

    Field* Class::DefineField(std::string name, Token::Class* type){
        Field* field = new Field(name, this, type);
        fields_.push_back(field);
        return field;
    }

    Field* Class::GetField(std::string name){
        for(auto& it : fields_){
            if(it->GetName() == name){
                return it;
            }
        }
        return nullptr;
    }

    Instance* Instance::New(Class* type){
        //TODO: Allocate here
        return new Instance(type);
    }
}