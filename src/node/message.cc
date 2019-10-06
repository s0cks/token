#include <glog/logging.h>
#include "node/message.h"
#include "block.h"

namespace Token{
    Message::Message(const Message& other):
            type_(other.GetType()),
            msg_(other.GetRaw()->New()){
        msg_->CopyFrom(*other.msg_);
    }

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
        SetType(static_cast<Type>(type));

        uint32_t msg_size;
        memcpy(&msg_size, &bytes[sizeof(uint32_t)], sizeof(uint32_t));
        LOG(INFO) << "decoded message size: " << msg_size;

        uint32_t msg_off = (sizeof(uint32_t) * 2);
        if(msg_size < 0) return false;
        if(msg_size == 0) return true;
        switch(GetType()){
            case Type::kPingMessage:{
                LOG(INFO) << "decoding PingMessage";
                msg_ = new Token::Messages::Node::Nonce();
                return GetRaw()->ParseFromArray(&bytes[msg_off], msg_size);
            }
            case Type::kPongMessage:{
                LOG(INFO) << "decoding PongMessage";
                msg_ = new Token::Messages::Node::Nonce();
                return GetRaw()->ParseFromArray(&bytes[msg_off], msg_size);
            }
            case Type::kVersionMessage:{
                LOG(INFO) << "decoding VersionMessage";
                msg_ = new Token::Messages::Node::Version();
                return GetRaw()->ParseFromArray(&bytes[msg_off], msg_size);
            }
            case Type::kGetHeadMessage:{
                LOG(INFO) << "decoding GetHeadMessage";
                return true;
            }
            case Type::kBlockMessage:{
                LOG(INFO) << "decoding block message";
                msg_ = new Token::Messages::Block();
                return GetRaw()->ParseFromArray(&bytes[msg_off], msg_size);
            }
            case Type::kGetBlockMessage:{
                LOG(INFO) << "decoding GetBlock";
                msg_ = new Token::Messages::Service::GetBlockRequest();
                return GetRaw()->ParseFromArray(&bytes[msg_off], msg_size);
            }
            case Type::kUnknownType: {
                LOG(ERROR) << "decoded unknown message type: " << type;
                return false;
            }
        }
    }

    std::string Message::ToString() const{
        std::stringstream stream;
        switch(GetType()){
            case Type::kUnknownType: return "Unknown";
            case Type::kPingMessage:{
                Messages::Node::Nonce* nonce = (Messages::Node::Nonce*)GetRaw();
                stream << "Ping(" << nonce->data() << ")";
                break;
            }
            case Type::kPongMessage:{
                Messages::Node::Nonce* nonce = (Messages::Node::Nonce*)GetRaw();
                stream << "Pong(" << nonce->data() << ")";
                break;
            }
            case Type::kBlockMessage:{
                Block* blk = Block::Decode(static_cast<Messages::Block*>(msg_));
                stream << "Block(" << blk->GetHash() << ")";
                break;
            }
            case Type::kGetHeadMessage: return "GetHead()";
            case Type::kGetBlockMessage:{
                Messages::Service::GetBlockRequest* gblock = (Messages::Service::GetBlockRequest*)GetRaw();
                stream << "GetBlock(" << gblock->hash() << ")";
                break;
            }
        }
        return stream.str();
    }

#define DEFINE_AS(Name, MType) \
    MType* Message::GetAs##Name(){ \
        return reinterpret_cast<MType*>(GetRaw()); \
    }
    FOR_EACH_TYPE(DEFINE_AS)
#undef DEFINE_AS

#define DEFINE_IS(Name, MType) \
    bool Message::Is##Name##Message(){ \
        return GetType() == Message::Type::k##Name##Message; \
    }
    FOR_EACH_TYPE(DEFINE_IS)
#undef DEFINE_IS
}