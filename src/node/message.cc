#include "node/message.h"

namespace Token{
    bool Message::Encode(uint8_t* bytes, size_t size){
        uint32_t type = static_cast<uint32_t>(GetType());
        uint32_t msg_size = static_cast<uint32_t>(GetMessageSize());
        memcpy(bytes, &type, sizeof(uint32_t));
        memcpy(&bytes[sizeof(uint32_t)], &msg_size, sizeof(uint32_t));
        if(msg_size == 0) return true;
        uint8_t msg_bytes[msg_size];
        GetRaw()->SerializeToArray(msg_bytes, msg_size);
        memcpy(&bytes[sizeof(uint32_t) * 2], msg_bytes, msg_size);
        return true;
    }

    bool Message::Decode(Token::ByteBuffer* bb){
        uint32_t type = bb->GetInt();
        std::cout << "Read Type: " << type << std::endl;
        SetType(static_cast<Type>(type));
        std::cout << "Message Type: " << static_cast<int>(GetType()) << std::endl;
        uint32_t size = bb->GetInt();
        std::cout << "Read: " << size << "/" << bb->WrittenBytes() << std::endl;
        if(size < 0) return false;
        if(size == 0) return true;

        uint8_t bytes[size];
        bb->GetBytes(bytes, sizeof(uint32_t) * 2, size);
        switch(GetType()){
            case Message::Type::kGetHeadMessage: return true;
            case Message::Type::kBlockMessage:{
                std::cout << "Decoding block message" << std::endl;
                msg_ = new Token::Messages::Block();
                return msg_->ParseFromArray(bytes, size);
            }
            case Message::Type::kUnknownType: return false;
        }
    }

    bool Message::DecodeBlockMessage(uint8_t* bytes, size_t size){
        msg_ = new Token::Messages::Block();
        return GetRaw()->ParseFromArray(bytes, size);
    }

    bool Message::DecodeGetHeadMessage(uint8_t* bytes, size_t size){
        return true;
    }

#define DEFINE_AS(Name, MType) \
    MType* Message::GetAs##Name##Message(){ \
        return reinterpret_cast<MType*>(GetRaw()); \
    }
    FOR_EACH_TYPE(DEFINE_AS)
#undef DEFINE_AS
}