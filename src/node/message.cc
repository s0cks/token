#include <glog/logging.h>
#include "node/message.h"
#include "block.h"

namespace Token{
    Message::Message(Token::ByteArray *bytes):
        type_(Message::Type::kUnknownType),
        raw_(nullptr){
        if(!Decode(bytes)){
            LOG(ERROR) << "couldn't decode message";
            type_ = Message::Type::kUnknownType;
            delete raw_;
            raw_ = nullptr;
        }
    }

    Message::Message(Token::Block* block):
        type_(Message::Type::kBlockMessage),
        raw_(new Messages::Block()){
        GetAsBlock()->CopyFrom(*block->GetRaw());
    }

    Message::~Message(){

    }

    bool Message::Encode(Token::ByteArray* bytes){
        bytes->Clear();
        bytes->Resize(GetMessageSize());
        bytes->PutUInt32(0, static_cast<uint32_t>(type_));
        bytes->PutUInt32(sizeof(uint32_t), GetMessageSize());
        return GetRaw()->SerializeToArray(bytes->data_, GetMessageSize());
    }

    bool Message::Decode(Token::ByteArray* bytes){
#define DEFINE_DECODE_MESSAGE(Name, MType) \
        case Message::Type::k##Name##Message:{ \
            raw_ = (new MType()); \
            return raw_->ParseFromArray(&bytes->data_[sizeof(uint32_t) * 2], size); \
        }

        uint32_t size = bytes->GetUInt32(sizeof(uint32_t));
        switch(type_ = static_cast<Type>(bytes->GetUInt32(0))){
            FOR_EACH_TYPE(DEFINE_DECODE_MESSAGE);
            case Message::Type::kUnknownType:
            default:{
                LOG(WARNING) << "unknown type";
                return false;
            }
        }
    }

    std::string Message::ToString(){
#define DEFINE_CHECK(Name, MType) \
        case Message::Type::k##Name##Message:{ \
            std::stringstream stream; \
            stream << #Name << "Message()"; \
            return stream.str(); \
        }
        switch(type_){
            FOR_EACH_TYPE(DEFINE_CHECK);
            default: return "Unknown Message";
        }
    }

#define DEFINE_AS(Name, MType) \
    MType* Message::GetAs##Name(){ \
        return reinterpret_cast<MType*>(raw_); \
    }
    FOR_EACH_TYPE(DEFINE_AS)
#undef DEFINE_AS

#define DEFINE_IS(Name, MType) \
    bool Message::Is##Name##Message(){ \
        return type_ == Message::Type::k##Name##Message; \
    }
    FOR_EACH_TYPE(DEFINE_IS)
#undef DEFINE_IS
}