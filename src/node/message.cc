#include "node/message.h"

namespace Token{
#define DEFINE_TYPECHECK(Name) \
    Name##Message* Name##Message::As##Name##Message(){ \
        return this; \
    }
    FOR_EACH_MESSAGE_TYPE(DEFINE_TYPECHECK)
#undef DEFINE_TYPECHECK

    Message* Message::Decode(uint8_t* bytes, size_t size){
        uint32_t rtype = 0;
        memcpy(&rtype, &bytes[Message::kTypeOffset], Message::kTypeLength);

        uint64_t rsize = 0;
        memcpy(&rsize, &bytes[Message::kSizeOffset], Message::kSizeLength);

        return Message::Decode(static_cast<Type>(rtype), &bytes[Message::kDataOffset], rsize);
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
            DECLARE_RAW_DECODE(GetData, Proto::BlockChainServer::GetData, "couldn't deserialize getdata from byte array");
            DECLARE_RAW_DECODE(Transaction, Proto::BlockChain::Transaction, "couldn't deserialize transaction from byte array");
            DECLARE_RAW_DECODE(Block, Proto::BlockChain::Block, "couldn't deserialize block from byte array");
            DECLARE_RAW_DECODE(Inventory, Proto::BlockChainServer::Inventory, "couldn't deserialize inventory from byte array");
            //DECLARE_RAW_DECODE(GetBlocks, Proto::BlockChainServer::BlockRange, "couldn't deserialize block range from byte array");
            DECLARE_RAW_DECODE(Prepare, Proto::BlockChainServer::Paxos, "couldn't deserialize prepare from byte array");
            DECLARE_RAW_DECODE(Promise, Proto::BlockChainServer::Paxos, "couldn't deserialize promise from byte array");
            DECLARE_RAW_DECODE(Commit, Proto::BlockChainServer::Paxos, "couldn't deserialize commit from byte array");
            DECLARE_RAW_DECODE(Version, Proto::BlockChainServer::Version, "couldn't deserialize version from byte array");
            DECLARE_RAW_DECODE(Verack, Proto::BlockChainServer::Verack, "couldn't deserialize verack from byte array");
            DECLARE_RAW_DECODE(Accepted, Proto::BlockChainServer::Paxos, "couldn't deserialize accepted from byte array");
            DECLARE_RAW_DECODE(Rejected, Proto::BlockChainServer::Paxos, "couldn't deserialize rejected from byte array");
            default:{
                LOG(ERROR) << "invalid message of type " << static_cast<uint8_t>(type) << " w/ size " << size;
                return nullptr;
            }
        }
    }
}