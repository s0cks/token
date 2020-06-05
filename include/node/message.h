#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "blockchain.pb.h"
#include "node.pb.h"
#include "service.pb.h"

#include "common.h"
#include "paxos.h"
#include "info.h"
#include "token.h"
#include "block_chain.h"

namespace Token{
#define FOR_EACH_MESSAGE_TYPE(V) \
    V(Version) \
    V(Verack) \
    V(Prepare) \
    V(Promise) \
    V(Commit) \
    V(Accepted) \
    V(Rejected) \
    V(GetData) \
    V(Block) \
    V(Transaction)

#define FORWARD_DECLARE(Name) class Name##Message;
    FOR_EACH_MESSAGE_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

    class ByteBuffer;

    class Message{
    public:
#define DECLARE_TYPE(Name) k##Name##Message,
        enum class Type{
            kUnknownMessage = 0,
            FOR_EACH_MESSAGE_TYPE(DECLARE_TYPE)
        };
#undef DECLARE_TYPE

        enum{
            kTypeOffset = 0,
            kTypeLength = 1,
            kSizeOffset = kTypeLength,
            kSizeLength = 4,
            kDataOffset = (kTypeLength + kSizeLength),
            kHeaderSize = kDataOffset,
            kByteBufferSize = 6144
        };
    public:
        virtual ~Message() = default;
        virtual std::string GetName() const = 0;
        virtual Type GetType() const = 0;
        virtual uint64_t GetSize() const = 0;
        virtual bool Encode(uint8_t* bytes, size_t size) const = 0;

#define DECLARE_TYPECHECK(Name) \
        virtual Name##Message* As##Name##Message(){ return nullptr; } \
        bool Is##Name##Message(){ return As##Name##Message() != nullptr; }
        FOR_EACH_MESSAGE_TYPE(DECLARE_TYPECHECK);
#undef DECLARE_TYPECHECK

        static Message* Decode(uint8_t* bytes, size_t size);
        static Message* Decode(Type type, uint8_t* bytes, uint32_t size);

        friend std::ostream& operator<<(std::ostream& stream, const Message& msg){
            stream << msg.GetName() << "(" << (Message::kHeaderSize + msg.GetSize()) << " Bytes)";
            return stream;
        }
    };

#define DECLARE_MESSAGE(Name) \
    public: \
        virtual Name##Message* As##Name##Message(); \
        virtual std::string GetName() const{ return #Name; } \
        virtual Type GetType() const{ return Type::k##Name##Message; }
#define DECLARE_MESSAGE_WITH_SIZE(Name, Size) \
    DECLARE_MESSAGE(Name); \
    virtual uint64_t GetSize() const{ return Size; }

    class HashMessage : public Message{
    protected:
        std::string hash_;

        HashMessage(const std::string& hash):
            hash_(hash){}
    public:
        virtual ~HashMessage() = default;

        uint64_t GetSize() const{
            return 64;
        }

        std::string GetHash(){
            return hash_;
        }

        bool Encode(uint8_t* bytes, uint64_t size) const{
            memcpy(bytes, hash_.data(), size);
            return true;
        }
    };

    class GetDataMessage : public HashMessage{
    public:
        GetDataMessage(const std::string& hash): HashMessage(hash){}
        GetDataMessage(const uint256_t& hash): HashMessage(HexString(hash)){}
        ~GetDataMessage(){}

        DECLARE_MESSAGE(GetData);
    };

    template<typename T>
    class ProtobufMessage : public Message{
    protected:
        T raw_;

        T GetRaw() const{
            return raw_;
        }

        ProtobufMessage():
            raw_(){}
        ProtobufMessage(const T& raw):
            raw_(raw){}
    public:
        virtual ~ProtobufMessage() = default;

        size_t GetSize() const{
            return GetRaw().ByteSizeLong();
        }

        bool Encode(uint8_t* bytes, uint64_t size) const{
            return GetRaw().SerializeToArray(bytes, size);
        }
    };

    class VersionMessage : public ProtobufMessage<Proto::BlockChainServer::Version>{
    public:
        VersionMessage(const Proto::BlockChainServer::Version& raw): ProtobufMessage(raw){}
        VersionMessage(const NodeInfo& info, const std::string& version=Token::GetVersion(), const uint64_t timestamp=GetCurrentTime(), const std::string& nonce=GenerateNonce()):
                ProtobufMessage(){
            raw_.set_node_id(info.GetNodeID());
            raw_.set_version(version);
            raw_.set_timestamp(timestamp);
            raw_.set_nonce(nonce);
        }
        ~VersionMessage(){}

        uint64_t GetTimestamp() const{
            return raw_.timestamp();
        }

        std::string GetVersion() const{
            return raw_.version();
        }

        std::string GetNonce() const{
            return raw_.nonce();
        }

        std::string GetID() const{
            return raw_.node_id();
        }

        DECLARE_MESSAGE(Version);
    };

    class VerackMessage : public ProtobufMessage<Proto::BlockChainServer::Verack>{
    public:
        VerackMessage(const Proto::BlockChainServer::Verack& raw): ProtobufMessage(raw){}
        VerackMessage(const NodeInfo& info, const std::string& version=Token::GetVersion(), const uint64_t timestamp=GetCurrentTime(), const std::string& nonce=GenerateNonce()):
                ProtobufMessage(){
            raw_.set_node_id(info.GetNodeID());
            (*raw_.mutable_callback()) << info.GetNodeAddress();
            raw_.set_version(version);
            raw_.set_timestamp(timestamp);
            raw_.set_nonce(nonce);
        }

        std::string GetID() const{
            return raw_.node_id();
        }

        NodeAddress GetCallbackAddress() const{
            return NodeAddress(raw_.callback());
        }

        DECLARE_MESSAGE(Verack);
    };

    class BasePaxosMessage : public ProtobufMessage<Proto::BlockChainServer::Paxos>{
    public:
        BasePaxosMessage(const Proto::BlockChainServer::Paxos& raw): ProtobufMessage(raw){}
        BasePaxosMessage(const NodeInfo& info, uint32_t height, const std::string& hash): ProtobufMessage(){
            raw_.set_node_id(info.GetNodeID());
            raw_.set_height(height);
            raw_.set_hash(hash);
        }
        BasePaxosMessage(const NodeInfo& info, const Block& block): ProtobufMessage(){
            raw_.set_node_id(info.GetNodeID());
            raw_.set_height(block.GetHeight());
            raw_.set_hash(HexString(block.GetHash()));
        }
        BasePaxosMessage(const NodeInfo& info, const Proposal& proposal): ProtobufMessage(){
            raw_.set_node_id(info.GetNodeID());
            raw_.set_height(proposal.GetHeight());
            raw_.set_hash(HexString(proposal.GetHash()));
        }
        virtual ~BasePaxosMessage() = default;

        std::string GetNodeID() const{
            return raw_.node_id();
        }

        uint256_t GetHash() const{
            return HashFromHexString(raw_.hash());
        }

        uint32_t GetHeight() const{
            return raw_.height();
        }
    };

    class PrepareMessage : public BasePaxosMessage{
    public:
        PrepareMessage(const Proto::BlockChainServer::Paxos& raw): BasePaxosMessage(raw){}
        PrepareMessage(const NodeInfo& info, uint32_t height, const std::string& hash): BasePaxosMessage(info, height, hash){}
        PrepareMessage(const NodeInfo& info, const Proposal& proposal): BasePaxosMessage(info, proposal){}
        ~PrepareMessage(){}

        DECLARE_MESSAGE(Prepare);
    };

    class PromiseMessage : public BasePaxosMessage{
    public:
        PromiseMessage(const Proto::BlockChainServer::Paxos& raw): BasePaxosMessage(raw){}
        PromiseMessage(const NodeInfo& info, uint32_t height, const std::string& hash): BasePaxosMessage(info, height, hash){}
        PromiseMessage(const NodeInfo& info, const Proposal& proposal): BasePaxosMessage(info, proposal){}
        ~PromiseMessage(){}

        DECLARE_MESSAGE(Promise);
    };

    class CommitMessage : public BasePaxosMessage{
    public:
        CommitMessage(const Proto::BlockChainServer::Paxos& raw): BasePaxosMessage(raw){}
        CommitMessage(const NodeInfo& info, uint32_t height, const std::string& hash): BasePaxosMessage(info, height, hash){}
        CommitMessage(const NodeInfo& info, const Proposal& proposal): BasePaxosMessage(info, proposal){}
        ~CommitMessage(){}

        DECLARE_MESSAGE(Commit);
    };

    class AcceptedMessage : public BasePaxosMessage{
    public:
        AcceptedMessage(const Proto::BlockChainServer::Paxos& raw): BasePaxosMessage(raw){}
        AcceptedMessage(const NodeInfo& info, uint32_t height, const std::string& hash): BasePaxosMessage(info, height, hash){}
        AcceptedMessage(const NodeInfo& info, const Proposal& proposal): BasePaxosMessage(info, proposal){}
        ~AcceptedMessage(){}

        DECLARE_MESSAGE(Accepted);
    };

    class RejectedMessage : public BasePaxosMessage{
    public:
        RejectedMessage(const Proto::BlockChainServer::Paxos& raw): BasePaxosMessage(raw){}
        RejectedMessage(const NodeInfo& info, uint32_t height, const std::string& hash): BasePaxosMessage(info, height, hash){}
        RejectedMessage(const NodeInfo& info, const Proposal& proposal): BasePaxosMessage(info, proposal){}
        ~RejectedMessage(){}

        DECLARE_MESSAGE(Rejected);
    };

    class TransactionMessage : public ProtobufMessage<Proto::BlockChain::Transaction>{
    public:
        TransactionMessage(const Proto::BlockChain::Transaction& raw): ProtobufMessage(raw){}
        TransactionMessage(const Transaction& tx):
            ProtobufMessage(){
            raw_ << tx;
        }
        ~TransactionMessage(){}

        Transaction* GetTransaction() const{
            return new Transaction(raw_);
        }

        DECLARE_MESSAGE(Transaction);
    };

    class BlockMessage : public ProtobufMessage<Proto::BlockChain::Block>{
    public:
        BlockMessage(const Proto::BlockChain::Block& raw): ProtobufMessage(raw){}
        BlockMessage(Block* block): ProtobufMessage(){
            raw_ << (*block);
        }
        ~BlockMessage(){}

        Block* GetBlock() const{
            return new Block(raw_);
        }

        DECLARE_MESSAGE(Block);
    };
}

#endif //TOKEN_MESSAGE_H