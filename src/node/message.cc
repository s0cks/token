#include <glog/logging.h>
#include <string>
#include "node/message.h"

namespace Token{
#define DEFINE_TYPECHECK(Name) \
    Name##Message* Name##Message::As##Name##Message(){ \
        return this; \
    }
    FOR_EACH_MESSAGE_TYPE(DEFINE_TYPECHECK)
#undef DEFINE_TYPECHECK

    void PingMessage::Encode(uint8_t* bytes, size_t size){
        memcpy(bytes, nonce_.data(), size);
    }

    void PongMessage::Encode(uint8_t* bytes, size_t size){
        memcpy(bytes, nonce_.data(), size);
    }

    void TransactionMessage::Encode(uint8_t* bytes, size_t size){
        if(!GetRaw().SerializeToArray(bytes, size)){
            LOG(ERROR) << "couldn't serialize transaction to byte array";
        }
    }

    void BlockMessage::Encode(uint8_t *bytes, size_t size){
        if(!GetRaw().SerializePartialToArray(bytes, size)){
            LOG(ERROR) << "couldn't serialize transaction to byte array";
        }
    }

    Message* Message::Decode(Type type, uint8_t* bytes, uint32_t size){
        switch(type){
            case Type::kPingMessage:{
                if(size != 64){
                    LOG(ERROR) << "ping nonce is " << size << "/64 characters";
                    return nullptr;
                }
                std::string nonce((char*)bytes, size);
                return new PingMessage(nonce);
            }
            case Type::kPongMessage:{
                if(size != 64){
                    LOG(ERROR) << "pong nonce is " << size << "/64 characters";
                    return nullptr;
                }
                std::string nonce((char*)bytes, size);
                return new PongMessage(nonce);
            }
            case Type::kTransactionMessage:{
                Messages::Transaction raw;
                if(!raw.ParseFromArray(bytes, size)){
                    LOG(ERROR) << "couldn't deserialize transaction from byte array";
                    return nullptr;
                }
                return new TransactionMessage(raw);
            }
            case Type::kBlockMessage:{
                Messages::Block raw;
                if(!raw.ParseFromArray(bytes, size)){
                    LOG(ERROR) << "couldn't deserialize block from byte array";
                    return nullptr;
                }
                return new BlockMessage(raw);
            }
            default:{
                LOG(ERROR) << "invalid message of type " << static_cast<uint8_t>(type) << " w/ size " << size;
                return nullptr;
            }
        }
    }
}