#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "blockchain.pb.h"
#include "node.pb.h"
#include "service.pb.h"

#include "common.h"
#include "token.h"
#include "block_chain.h"

namespace Token{
#define FOR_EACH_MESSAGE_TYPE(V) \
    V(Ping) \
    V(Pong) \
    V(Version) \
    V(Verack) \
    V(Block) \
    V(Transaction) \
    V(Inventory) \
    V(GetData) \
    V(GetBlocks)


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
    public:
        virtual ~Message() = default;
        virtual std::string GetName() const = 0;
        virtual Type GetType() const = 0;
        virtual uint64_t GetSize() const = 0;
        virtual bool Encode(uint8_t* bytes, size_t size) = 0;

#define DECLARE_TYPECHECK(Name) \
        virtual Name##Message* As##Name##Message(){ return nullptr; } \
        bool Is##Name##Message(){ return As##Name##Message() != nullptr; }
        FOR_EACH_MESSAGE_TYPE(DECLARE_TYPECHECK);
#undef DECLARE_TYPECHECK

        static Message* Decode(ByteBuffer* bb);
        static Message* Decode(Type type, uint8_t* bytes, uint32_t size);
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

        bool Encode(uint8_t* bytes, uint64_t size){
            memcpy(bytes, hash_.data(), size);
            return true;
        }
    };

    class PingMessage : public HashMessage{
    public:
        PingMessage(const std::string& nonce=GenerateNonce()): HashMessage(nonce){}
        ~PingMessage(){}

        std::string GetNonce() const{
            return hash_;
        }

        DECLARE_MESSAGE(Ping);
    };

    class PongMessage : public HashMessage{
    public:
        PongMessage(const std::string& nonce=GenerateNonce()): HashMessage(nonce){}
        PongMessage(const PingMessage& ping): HashMessage(ping.GetNonce()){}
        ~PongMessage(){}

        std::string GetNonce() const{
            return hash_;
        }

        DECLARE_MESSAGE(Pong);
    };

    class GetDataMessage : public HashMessage{
    public:
        GetDataMessage(const std::string& hash): HashMessage(hash){}
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

        bool Encode(uint8_t* bytes, uint64_t size){
            return GetRaw().SerializeToArray(bytes, size);
        }
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
        BlockMessage(const Block& block):
            ProtobufMessage(){
            raw_ << block;
        }
        ~BlockMessage(){}

        Block* GetBlock() const{
            return new Block(raw_);
        }

        DECLARE_MESSAGE(Block);
    };

    class GetBlocksMessage : public ProtobufMessage<Proto::BlockChainServer::BlockRange>{
    public:
        GetBlocksMessage(const Proto::BlockChainServer::BlockRange& raw): ProtobufMessage(raw){}
        GetBlocksMessage(const std::string& first, const std::string& last):
            ProtobufMessage(){
            raw_.set_first(first);
            raw_.set_last(last);
        }
        ~GetBlocksMessage(){}

        uint256_t GetFirst() const{
            return HashFromHexString(raw_.first());
        }

        uint256_t GetLast() const{
            return HashFromHexString(raw_.last());
        }

        DECLARE_MESSAGE(GetBlocks);
    };

    class VersionMessage : public ProtobufMessage<Proto::BlockChainServer::Version>{
    public:
        VersionMessage(const Proto::BlockChainServer::Version& raw): ProtobufMessage(raw){}
        VersionMessage(const std::string& nonce=GenerateNonce()):
            ProtobufMessage(){
            raw_.set_version(Token::GetVersion());
            raw_.set_timestamp(GetCurrentTime());
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

        DECLARE_MESSAGE(Version);
    };

    class VerackMessage : public ProtobufMessage<Proto::BlockChainServer::Verack>{
    public:
        VerackMessage(const Proto::BlockChainServer::Verack& raw): ProtobufMessage(raw){}
        VerackMessage(const std::string& nonce=GenerateNonce()):
            ProtobufMessage(){
            raw_.set_nonce(nonce);
            raw_.set_max_block(BlockChain::GetHeight());
        }

        uint64_t GetMaxBlock() const{
            return raw_.max_block();
        }

        DECLARE_MESSAGE(Verack);
    };

    class InventoryMessage : public ProtobufMessage<Proto::BlockChainServer::Inventory>{
    public:
        enum class InventoryItemType{
            kNone = 0,
            kError = 1 << 1,
            kBlock = 1 << 2,
            kTransaction = 1 << 3,
        };

        InventoryMessage(const Proto::BlockChainServer::Inventory& raw): ProtobufMessage(raw){}
        ~InventoryMessage(){}

        bool GetHashes(std::vector<std::string>& hashes){
            for(auto& it : raw_.items()){
                hashes.push_back(it);
            }
            return hashes.size() > 0;
        }

        DECLARE_MESSAGE(Inventory);
    };
}

#endif //TOKEN_MESSAGE_H