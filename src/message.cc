#include <glog/logging.h>
#include <block.h>
#include "message.h"

namespace Token{
    bool Message::Encode(uint8_t* bytes, size_t size){
        uint32_t type = static_cast<uint32_t>(GetType());
        uint32_t msg_size = static_cast<uint32_t>(GetMessageSize());
        LOG(WARNING) << "setting message bytes to 0";
        memset(bytes, 0, size);
        LOG(WARNING) << "setting message type to " << type;
        memcpy(bytes, &type, sizeof(uint32_t));
        LOG(WARNING) << "setting message size to " << msg_size;
        memcpy(&bytes[sizeof(uint32_t)], &msg_size, sizeof(uint32_t));
        if(msg_size == 0) return true;
        uint8_t msg_bytes[msg_size];
        GetRaw()->SerializeToArray(msg_bytes, msg_size);
        LOG(WARNING) << "setting message bytes to raw message";
        memcpy(&bytes[(sizeof(uint32_t) * 2)], msg_bytes, msg_size);
        return true;
    }

    bool Message::Decode(uint8_t *bytes, size_t size){
        uint32_t type;
        memcpy(&type, bytes, sizeof(uint32_t));
        LOG(INFO) << "decoded type: " << type;

        uint32_t msg_size;
        memcpy(&msg_size, &bytes[sizeof(uint32_t)], sizeof(uint32_t));
        LOG(INFO) << "decoded message size: " << msg_size;

        if(msg_size < 0) return false;
        if(msg_size == 0) return true;
        switch((type_ = static_cast<Type>(type))){
            case Type::kGetHeadMessage:{
                LOG(INFO) << "decoding GetHeadMessage";
                return true;
            }
            case Type::kBlockMessage:{
                LOG(INFO) << "decoding block message";
                msg_ = new Token::Messages::Block();
                return GetRaw()->ParseFromArray(&bytes[(sizeof(uint32_t) * 2)], msg_size);
            }
            case Type::kUnknownType: {
                LOG(ERROR) << "decoded unknown message type: " << type;
                return false;
            }
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

    Message::Message(const Message& other):
        type_(other.GetType()),
        msg_(other.GetRaw()->New()){
        msg_->CopyFrom(*other.msg_);
    }

    std::string Message::ToString() const{
        std::stringstream stream;
        switch(GetType()){
            case Type::kUnknownType: return "Unknown";
            case Type::kBlockMessage:{
                Block* blk = new Block(msg_);
                stream << "Block(" << blk->GetHash() << ")";
                return stream.str();
            }
            case Type::kGetHeadMessage: return "GetHead()";
        }
    }
}