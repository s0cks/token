#include <glog/logging.h>
#include <string>
#include "node/message.h"
#include "node/buffer.h"
#include "layout.h"

namespace Token{
#define DEFINE_TYPECHECK(Name) \
    Name##Message* Name##Message::As##Name##Message(){ \
        return this; \
    }
    FOR_EACH_MESSAGE_TYPE(DEFINE_TYPECHECK)
#undef DEFINE_TYPECHECK

    Message* Message::Decode(ByteBuffer* bb){
        Message::Type type = static_cast<Message::Type>(bb->GetInt());
        uint64_t size = bb->GetLong();
        uint8_t bytes[size];
        bb->GetBytes(bytes, size);
        return Decode(type, bytes, size);
    }

#define DECLARE_HASH_DECODE(Name, ErrorMessage) \
    case Type::k##Name##Message:{ \
        if(size != 64){ \
            LOG(ERROR) << ErrorMessage; \
            return nullptr; \
        } \
        std::string hash((char*)bytes, size); \
        return new Name##Message(hash); \
    }
#define DECLARE_RAW_DECODE(Name, RawType, ErrorMessage) \
    case Type::k##Name##Message:{ \
        RawType raw; \
        if(!raw.ParseFromArray(bytes, size)){ \
            LOG(ERROR) << ErrorMessage; \
            return nullptr; \
        } \
        return new Name##Message(raw); \
    }

    Message* Message::Decode(Type type, uint8_t* bytes, uint32_t size){
        switch(type){
            DECLARE_HASH_DECODE(Ping, "couldn't deserialize ping from byte array");
            DECLARE_HASH_DECODE(Pong, "couldn't deserialize pong from byte array");
            DECLARE_HASH_DECODE(GetData, "couldn't deserialize getdata from byte array");
            DECLARE_RAW_DECODE(RequestVote, Proto::BlockChainServer::Action, "couldn't deserialize request vote from byte array");
            DECLARE_RAW_DECODE(Vote, Proto::BlockChainServer::Action, "couldn't deserialize vote from byte array");
            DECLARE_RAW_DECODE(Election, Proto::BlockChainServer::Action, "couldn't deserialize election from byte array");
            DECLARE_RAW_DECODE(Commit, Proto::BlockChainServer::Action, "couldn't deserialize commit from byte array");
            DECLARE_RAW_DECODE(Transaction, Proto::BlockChain::Transaction, "couldn't deserialize transaction from byte array");
            DECLARE_RAW_DECODE(Block, Proto::BlockChain::Block, "couldn't deserialize block from byte array");
            DECLARE_RAW_DECODE(Inventory, Proto::BlockChainServer::Inventory, "couldn't deserialize inventory from byte array");
            DECLARE_RAW_DECODE(GetBlocks, Proto::BlockChainServer::BlockRange, "couldn't deserialize block range from byte array");
            DECLARE_RAW_DECODE(Version, Proto::BlockChainServer::Version, "couldn't deserialize version from byte array");
            DECLARE_RAW_DECODE(Verack, Proto::BlockChainServer::Verack, "couldn't deserialize verack from byte array");
            default:{
                LOG(ERROR) << "invalid message of type " << static_cast<uint8_t>(type) << " w/ size " << size;
                return nullptr;
            }
        }
    }
}