#include "message.h"
#include "proposal.h"
#include "proposer.h"
#include "block_miner.h"

namespace Token{
    const uint32_t GetBlocksMessage::kMaxNumberOfBlocks = 250;

#define DECLARE_RAW_DECODE(Name, RawType, ErrorMessage) \
    case MessageType::k##Name##MessageType:{ \
        RawType raw; \
        if(!raw.ParseFromArray(bytes, size)){ \
            LOG(ERROR) << ErrorMessage; \
            return nullptr; \
        } \
        return Name##Message::NewInstance(raw).CastTo<Message>(); \
    }

#define DECLARE_HASH_DECODE(Name, ErrorMessage) \
case MessageType::k##Name##MessageType:{ \
    uint256_t hash(bytes); \
    return Name##Message::NewInstance(hash).CastTo<Message>(); \
}

    Handle<Message> Message::Decode(Message::MessageType type, uintptr_t size, uint8_t* bytes){
        switch(type){
            case MessageType::kGetUnclaimedTransactionsMessageType:{
                std::string user((char*)bytes, size);
                return GetUnclaimedTransactionsMessage::NewInstance(user).CastTo<Message>();
            }
            DECLARE_RAW_DECODE(GetBlocks, Proto::BlockChainServer::GetBlocks, "couldn't deserialize getblocks from byte array");
            DECLARE_RAW_DECODE(GetData, Proto::BlockChainServer::Inventory, "couldn't deserialize getdata from byte array");
            DECLARE_RAW_DECODE(Transaction, Proto::BlockChain::Transaction, "couldn't deserialize transaction from byte array");
            DECLARE_RAW_DECODE(Block, Proto::BlockChain::Block, "couldn't deserialize block from byte array");
            DECLARE_RAW_DECODE(Inventory, Proto::BlockChainServer::Inventory, "couldn't deserialize inventory from byte array");
            DECLARE_RAW_DECODE(Prepare, Proto::BlockChainServer::Proposal, "couldn't deserialize prepare from byte array");
            DECLARE_RAW_DECODE(Promise, Proto::BlockChainServer::Proposal, "couldn't deserialize promise from byte array");
            DECLARE_RAW_DECODE(Commit, Proto::BlockChainServer::Proposal, "couldn't deserialize commit from byte array");
            DECLARE_RAW_DECODE(Version, Proto::BlockChainServer::Version, "couldn't deserialize version from byte array");
            DECLARE_RAW_DECODE(Verack, Proto::BlockChainServer::Verack, "couldn't deserialize verack from byte array");
            DECLARE_RAW_DECODE(Accepted, Proto::BlockChainServer::Proposal, "couldn't deserialize accepted from byte array");
            DECLARE_RAW_DECODE(Rejected, Proto::BlockChainServer::Proposal, "couldn't deserialize rejected from byte array");
            DECLARE_RAW_DECODE(NotFound, Proto::BlockChainServer::NotFound, "couldn't deserialize notfound from byte array");
            default:{
                LOG(ERROR) << "invalid message of type " << static_cast<uint8_t>(type) << " w/ size " << size;
                return nullptr;
            }
        }
    }

    PaxosMessage::PaxosMessage(const std::string& node, Proposal* proposal):
        ProtobufMessage<Proto::BlockChainServer::Proposal>(){
        raw_.set_node_id(node);
        raw_.set_hash(HexString(proposal->GetHash()));
    }


    Handle<Proposal> PaxosMessage::GetProposal() const{
        if(ProposerThread::HasProposal()){
            Handle<Proposal> proposal = ProposerThread::GetProposal();
            if(proposal->GetHeight() == GetHeight() &&
                proposal->GetHash() == GetHash()){
                return proposal;
            }

            LOG(WARNING) << "current proposal #" << proposal->GetHeight() << "(" << proposal->GetHash() << ") is invalid";
            LOG(WARNING) << "expected proposal #" << GetHeight() << "(" << GetHash() << ")";
        }
        return Proposal::NewInstance(GetHeight(), GetHash(), GetProposer());
    }
}