#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "token.h"
#include "common.h"
#include "node_info.h"
#include "node.h"
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
    V(GetBlocks) \
    V(Block) \
    V(Transaction) \
    V(Inventory) \
    V(NotFound) \
    V(Test)

#define FORWARD_DECLARE(Name) class Name##Message;
    FOR_EACH_MESSAGE_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

    class Message : public Object{
    public:
        enum MessageType{
            kUnknownMessageType = 0,
#define DECLARE_MESSAGE_TYPE(Name) k##Name##MessageType,
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_TYPE)
#undef DECLARE_MESSAGE_TYPE
        };

        enum{
            kTypeOffset = 0,
            kTypeLength = 1,
            kSizeOffset = kTypeLength,
            kSizeLength = 4,
            kDataOffset = (kTypeLength + kSizeLength),
            kHeaderSize = kDataOffset,
            kByteBufferSize = 6144
        };
    protected:
        Message(){}
    public:
        virtual ~Message() = default;
        virtual uintptr_t GetMessageSize() const = 0;
        virtual const char* GetName() const = 0;
        virtual bool Encode(uint8_t* bytes, uintptr_t size) const = 0;
        virtual MessageType GetMessageType() const = 0;

        std::string ToString() const{
            return std::string(GetName());
        }

#define DECLARE_TYPECHECK(Name) \
    bool Is##Name##Message(){ return GetMessageType() == Message::k##Name##MessageType; }
        FOR_EACH_MESSAGE_TYPE(DECLARE_TYPECHECK)
#undef DECLARE_TYPECHECK

        static Message* Decode(MessageType type, uintptr_t size, uint8_t* bytes);
    };

    class HashMessage : public Message{
    protected:
        uint256_t hash_;

        HashMessage(const uint256_t& hash):
            Message(),
            hash_(hash){}

        virtual bool Encode(uint8_t* bytes, uintptr_t size) const{
            memcpy(bytes, hash_.data(), size);
            return true;
        }
    public:
        virtual ~HashMessage() = default;

        uint256_t GetHash() const{
            return hash_;
        }

        virtual uintptr_t GetMessageSize() const{
            return 64; // bytes
        }
    };

#define DECLARE_MESSAGE(Name) \
    public: \
        virtual MessageType GetMessageType() const{ return Message::k##Name##MessageType; } \
        virtual const char* GetName() const{ return #Name; }

    template<typename T>
    class ProtobufMessage : public Message{
    protected:
        T raw_;

        T GetRaw() const{
            return raw_;
        }

        ProtobufMessage():
            Message(),
            raw_(){}
        ProtobufMessage(const T& raw):
            Message(),
            raw_(raw){}
    public:
        virtual ~ProtobufMessage() = default;

        uintptr_t GetAllocationSize() const{
            return Message::kHeaderSize + GetMessageSize();
        }

        uintptr_t GetMessageSize() const{
            return GetRaw().ByteSizeLong();
        }

        bool Encode(uint8_t* bytes, uintptr_t size) const{
            return GetRaw().SerializeToArray(bytes, size);
        }
    };

    class TestMessage : public HashMessage{
    private:
        TestMessage(const uint256_t& hash): HashMessage(hash){}
    public:
        ~TestMessage() = default;

        DECLARE_MESSAGE(Test);

        static TestMessage* NewInstance(const uint256_t& hash){
            TestMessage* instance = (TestMessage*)Allocator::Allocate(sizeof(TestMessage));
            return new (instance)TestMessage(hash);
        }
    };

    class VersionMessage : public ProtobufMessage<Proto::BlockChainServer::Version>{
    public:
        typedef Proto::BlockChainServer::Version RawType;
    private:
        VersionMessage(const RawType& raw): ProtobufMessage(raw){}
        VersionMessage(const NodeInfo& info, uint32_t timestamp, const std::string& nonce, const BlockHeader& head): ProtobufMessage(){
            (*raw_.mutable_info()) << info;
            raw_.set_version(Token::GetVersion());
            raw_.set_timestamp(timestamp);
            raw_.set_nonce(nonce);
            raw_.set_head(HexString(head.GetHash()));
            raw_.set_height(head.GetHeight());
        }
    public:
        ~VersionMessage(){}

        uint32_t GetTimestamp() const{
            return raw_.timestamp();
        }

        std::string GetVersion() const{
            return raw_.version();
        }

        std::string GetNonce() const{
            return raw_.nonce();
        }

        NodeInfo GetInfo() const{
            return NodeInfo(raw_.info());
        }

        uint32_t GetHeight() const{
            return raw_.height();
        }

        uint256_t GetHead() const{
            return HashFromHexString(raw_.head());
        }

        DECLARE_MESSAGE(Version);

        static VersionMessage* NewInstance(const NodeInfo& info, const std::string& nonce=GenerateNonce(), const BlockHeader& head=BlockChain::GetHead(), uint32_t timestamp=GetCurrentTime()){
            VersionMessage* instance = (VersionMessage*)Allocator::Allocate(sizeof(VersionMessage), Object::kMessage);
            new (instance)VersionMessage(info, timestamp, nonce, head);
            return instance;
        }

        static VersionMessage* NewInstance(const RawType& raw){
            VersionMessage* instance = (VersionMessage*)Allocator::Allocate(sizeof(VersionMessage), Object::kMessage);
            new (instance)VersionMessage(raw);
            return instance;
        }
    };

    class VerackMessage : public ProtobufMessage<Proto::BlockChainServer::Verack>{
    public:
        typedef Proto::BlockChainServer::Verack RawType;
    private:
        VerackMessage(const RawType& raw): ProtobufMessage(raw){}
        VerackMessage(const NodeInfo& info, const std::string& nonce, uint32_t timestamp): ProtobufMessage(){
            (*raw_.mutable_info()) << info;
            raw_.set_version(Token::GetVersion());
            raw_.set_timestamp(timestamp);
            raw_.set_nonce(nonce);
        }
    public:
        NodeInfo GetInfo() const{
            return NodeInfo(raw_.info());
        }

        std::string GetID() const{
            return GetInfo().GetNodeID();
        }

        NodeAddress GetCallbackAddress() const{
            return GetInfo().GetNodeAddress();
        }

        DECLARE_MESSAGE(Verack);

        static VerackMessage* NewInstance(const NodeInfo& info, const std::string& nonce=GenerateNonce(), uint32_t timestamp=GetCurrentTime()){
            VerackMessage* instance = (VerackMessage*)Allocator::Allocate(sizeof(VerackMessage), Object::kMessage);
            new (instance)VerackMessage(info, nonce, timestamp);
            return instance;
        }

        static VerackMessage* NewInstance(const RawType& raw){
            VerackMessage* instance = (VerackMessage*)Allocator::Allocate(sizeof(VerackMessage), Object::kMessage);
            new (instance)VerackMessage(raw);
            return instance;
        }
    };

    class Proposal;
    class PaxosMessage : public ProtobufMessage<Proto::BlockChainServer::Proposal>{
    protected:
        PaxosMessage(const Proto::BlockChainServer::Proposal& raw): ProtobufMessage(raw){}
        PaxosMessage(const NodeInfo& node, Proposal* proposal);
    public:
        virtual ~PaxosMessage() = default;

        Proposal* GetProposal() const;

        //TODO:
        // - NodeInfo GetProposer()
        // - NodeInfo GetSubmitter()
        NodeInfo GetProposer() const{
            return NodeInfo(raw_.info());
        }

        uint256_t GetHash() const{
            return HashFromHexString(raw_.hash());
        }
    };

    class PrepareMessage : public PaxosMessage{
    public:
        typedef Proto::BlockChainServer::Proposal RawType;
    private:
        PrepareMessage(const RawType& raw): PaxosMessage(raw){}
        PrepareMessage(const NodeInfo& info, Proposal* proposal): PaxosMessage(info, proposal){}
    public:
        ~PrepareMessage(){}

        DECLARE_MESSAGE(Prepare);

        static PrepareMessage* NewInstance(Proposal* proposal, const NodeInfo& info=Node::GetInfo()){
            PrepareMessage* instance = (PrepareMessage*)Allocator::Allocate(sizeof(PrepareMessage), Object::kMessage);
            new (instance)PrepareMessage(info, proposal);
            return instance;
        }

        static PrepareMessage* NewInstance(const RawType& raw){
            PrepareMessage* instance = (PrepareMessage*)Allocator::Allocate(sizeof(PrepareMessage), Object::kMessage);
            new (instance)PrepareMessage(raw);
            return instance;
        }
    };

    class PromiseMessage : public PaxosMessage{
    public:
        typedef Proto::BlockChainServer::Proposal RawType;
    private:
        PromiseMessage(const RawType& raw): PaxosMessage(raw){}
        PromiseMessage(const NodeInfo& info, Proposal* proposal): PaxosMessage(info, proposal){}
    public:
        ~PromiseMessage(){}

        DECLARE_MESSAGE(Promise);

        static PromiseMessage* NewInstance(Proposal* proposal, const NodeInfo& info=Node::GetInfo()){
            PromiseMessage* instance = (PromiseMessage*)Allocator::Allocate(sizeof(PromiseMessage));
            new (instance)PromiseMessage(info, proposal);
            return instance;
        }

        static PromiseMessage* NewInstance(const RawType& raw){
            PromiseMessage* instance = (PromiseMessage*)Allocator::Allocate(sizeof(PromiseMessage), Object::kMessage);
            new (instance)PromiseMessage(raw);
            return instance;
        }
    };

    class CommitMessage : public PaxosMessage{
    public:
        typedef Proto::BlockChainServer::Proposal RawType;
    private:
        CommitMessage(const RawType& raw): PaxosMessage(raw){}
        CommitMessage(const NodeInfo& info, Proposal* proposal): PaxosMessage(info, proposal){}
    public:
        ~CommitMessage(){}

        DECLARE_MESSAGE(Commit);

        static CommitMessage* NewInstance(Proposal* proposal, const NodeInfo& info=Node::GetInfo()){
            CommitMessage* instance = (CommitMessage*)Allocator::Allocate(sizeof(CommitMessage), Object::kMessage);
            new (instance)CommitMessage(info, proposal);
            return instance;
        }

        static CommitMessage* NewInstance(const RawType& raw){
            CommitMessage* instance = (CommitMessage*)Allocator::Allocate(sizeof(CommitMessage), Object::kMessage);
            new (instance)CommitMessage(raw);
            return instance;
        }
    };

    class AcceptedMessage : public PaxosMessage{
    public:
        typedef Proto::BlockChainServer::Proposal RawType;
    private:
        AcceptedMessage(const RawType& raw): PaxosMessage(raw){}
        AcceptedMessage(const NodeInfo& info, Proposal* proposal): PaxosMessage(info, proposal){}
    public:
        ~AcceptedMessage(){}

        DECLARE_MESSAGE(Accepted);

        static AcceptedMessage* NewInstance(Proposal* proposal, const NodeInfo& info=Node::GetInfo()){
            AcceptedMessage* instance = (AcceptedMessage*)Allocator::Allocate(sizeof(AcceptedMessage), Object::kMessage);
            new (instance)AcceptedMessage(info, proposal);
            return instance;
        }

        static AcceptedMessage* NewInstance(const RawType& raw){
            AcceptedMessage* instance = (AcceptedMessage*)Allocator::Allocate(sizeof(AcceptedMessage), Object::kMessage);
            new (instance)AcceptedMessage(raw);
            return instance;
        }
    };

    class RejectedMessage : public PaxosMessage{
    public:
        typedef Proto::BlockChainServer::Proposal RawType;
    private:
        RejectedMessage(const RawType& raw): PaxosMessage(raw){}
        RejectedMessage(const NodeInfo& info, Proposal* proposal): PaxosMessage(info, proposal){}
    public:
        ~RejectedMessage(){}

        DECLARE_MESSAGE(Rejected);

        static RejectedMessage* NewInstance(Proposal* proposal, const NodeInfo& info=Node::GetInfo()){
            RejectedMessage* instance = (RejectedMessage*)Allocator::Allocate(sizeof(RejectedMessage));
            new (instance)RejectedMessage(info, proposal);
            return instance;
        }

        static RejectedMessage* NewInstance(const RawType& raw){
            RejectedMessage* instance = (RejectedMessage*)Allocator::Allocate(sizeof(RejectedMessage));
            new (instance)RejectedMessage(raw);
            return instance;
        }
    };

    class TransactionMessage : public ProtobufMessage<Proto::BlockChain::Transaction>{
    public:
        typedef Proto::BlockChain::Transaction RawType;
    private:
        TransactionMessage(Transaction* tx): ProtobufMessage(){
            if(!tx->Encode(raw_)){
                LOG(WARNING) << "couldn't encode transaction to message!";
            }
        }

        TransactionMessage(const RawType& raw): ProtobufMessage(raw){}
    public:
        ~TransactionMessage(){}

        Transaction* GetTransaction() const{
            return Transaction::NewInstance(raw_);
        }

        DECLARE_MESSAGE(Transaction);

        static TransactionMessage* NewInstance(Transaction* tx){
            TransactionMessage* instance = (TransactionMessage*)Allocator::Allocate(sizeof(TransactionMessage));
            new (instance)TransactionMessage(tx);
            return instance;
        }

        static TransactionMessage* NewInstance(const RawType& raw){
            TransactionMessage* instance = (TransactionMessage*)Allocator::Allocate(sizeof(TransactionMessage), Object::kMessage);
            new (instance)TransactionMessage(raw);
            return instance;
        }
    };

    class BlockMessage : public ProtobufMessage<Proto::BlockChain::Block>{
    public:
        typedef Proto::BlockChain::Block RawType;
    private:
        BlockMessage(Block* blk): ProtobufMessage(){
            if(!blk->Encode(raw_)){
                LOG(WARNING) << "couldn't encode block to message";
            }
        }
        BlockMessage(const RawType& raw): ProtobufMessage(raw){}
    public:
        ~BlockMessage(){}

        Block* GetBlock() const{
            return Block::NewInstance(raw_);
        }

        DECLARE_MESSAGE(Block);

        static BlockMessage* NewInstance(Block* blk){
            BlockMessage* instance = (BlockMessage*)Allocator::Allocate(sizeof(BlockMessage), Object::kMessage);
            new (instance)BlockMessage(blk);
            return instance;
        }

        static BlockMessage* NewInstance(const RawType& raw){
            BlockMessage* instance = (BlockMessage*)Allocator::Allocate(sizeof(BlockMessage), Object::kMessage);
            new (instance)BlockMessage(raw);
            return instance;
        }
    };

    class InventoryItem{
    public:
        typedef Proto::BlockChainServer::InventoryItem RawType;

        enum Type{
            kTransaction,
            kBlock
        };
    private:
        Type type_;
        uint256_t hash_;
    public:
        InventoryItem(const RawType& raw):
            type_(static_cast<Type>(raw.type())),
            hash_(HashFromHexString(raw.hash())){}
        InventoryItem(Type type, const uint256_t& hash):
            type_(type),
            hash_(hash){}
        InventoryItem(Transaction* tx): InventoryItem(kTransaction, tx->GetSHA256Hash()){}
        InventoryItem(Block* blk): InventoryItem(kBlock, blk->GetSHA256Hash()){}
        InventoryItem(const BlockHeader& blk): InventoryItem(kBlock, blk.GetHash()){}
        InventoryItem(const InventoryItem& item):
            type_(item.type_),
            hash_(item.hash_){}
        ~InventoryItem(){}

        Type GetType() const{
            return type_;
        }

        uint256_t GetHash() const{
            return hash_;
        }

        bool ItemExists() const{
            switch(type_){
                case kTransaction: return TransactionPool::HasTransaction(hash_);
                case kBlock: return BlockChain::HasBlock(hash_);
                default: return false;
            }
        }

        bool IsBlock() const{
            return type_ == kBlock;
        }

        bool IsTransaction() const{
            return type_ == kTransaction;
        }

        InventoryItem& operator=(const InventoryItem& other){
            type_ = other.type_;
            hash_ = other.hash_;
            return (*this);
        }

        friend bool operator==(const InventoryItem& a, const InventoryItem& b){
            return a.type_ == b.type_ &&
                    a.hash_ == b.hash_;
        }

        friend bool operator!=(const InventoryItem& a, const InventoryItem& b){
            return !operator==(a, b);
        }

        friend RawType& operator<<(RawType& raw, const InventoryItem& item){
            raw.set_type(item.type_);
            raw.set_hash(HexString(item.hash_));
            return raw;
        }

        friend std::ostream& operator<<(std::ostream& stream, const InventoryItem& item){
            if(item.IsBlock()){
                stream << "Block(" << item.GetHash() << ")";
            } else if(item.IsTransaction()){
                stream << "Transaction(" << item.GetHash() << ")";
            } else{
                stream << "Unknown(" << item.GetHash() << ")";
            }
            return stream;
        }
    };

    class InventoryMessage : public ProtobufMessage<Proto::BlockChainServer::Inventory>{
    public:
        typedef Proto::BlockChainServer::Inventory RawType;
    private:
        InventoryMessage(const Proto::BlockChainServer::Inventory& raw): ProtobufMessage(raw){}
        InventoryMessage(std::vector<InventoryItem>& items): ProtobufMessage(){
            if(items.size() == 0){
                LOG(INFO) << "no items!";
                return;
            }

            for(auto& it : items){
                InventoryItem::RawType* item = raw_.add_items();
                (*item) << it;
            }
        }
    public:

        ~InventoryMessage(){}

        uint32_t GetNumberOfItems(){
            return raw_.items_size();
        }

        bool GetItems(std::vector<InventoryItem>& items){
            for(auto& it : raw_.items()){
                items.push_back(InventoryItem(it));
            }
            return items.size() > 0;
        }

        DECLARE_MESSAGE(Inventory);

        static InventoryMessage* NewInstance(std::vector<InventoryItem>& items){
            InventoryMessage* instance = (InventoryMessage*)Allocator::Allocate(sizeof(InventoryMessage), Object::kMessage);
            new (instance)InventoryMessage(items);
            return instance;
        }

        static InventoryMessage* NewInstance(const RawType& raw){
            InventoryMessage* instance = (InventoryMessage*)Allocator::Allocate(sizeof(InventoryMessage));
            new (instance)InventoryMessage(raw);
            return instance;
        }
    };

    class GetDataMessage : public ProtobufMessage<Proto::BlockChainServer::Inventory>{
    public:
        typedef Proto::BlockChainServer::Inventory RawType;
    private:
        GetDataMessage(const Proto::BlockChainServer::Inventory& raw): ProtobufMessage(raw){}
        GetDataMessage(std::vector<InventoryItem>& items): ProtobufMessage(){
            for(auto& it : items){
                InventoryItem::RawType* item = raw_.add_items();
                (*item) << it;
            }
        }
    public:
        ~GetDataMessage(){}

        bool GetItems(std::vector<InventoryItem>& items){
            for(auto& it : raw_.items()){
                items.push_back(InventoryItem(it));
            }
            return items.size() > 0;
        }

        DECLARE_MESSAGE(GetData);

        static GetDataMessage* NewInstance(std::vector<InventoryItem>& items){
            GetDataMessage* instance = (GetDataMessage*)Allocator::Allocate(sizeof(GetDataMessage));
            new (instance)GetDataMessage(items);
            return instance;
        }

        static GetDataMessage* NewInstance(const RawType& raw){
            GetDataMessage* instance = (GetDataMessage*)Allocator::Allocate(sizeof(GetDataMessage), Object::kMessage);
            new (instance)GetDataMessage(raw);
            return instance;
        }
    };

    class GetBlocksMessage : public ProtobufMessage<Proto::BlockChainServer::GetBlocks>{
    public:
        static const uint32_t kMaxNumberOfBlocks;

        typedef Proto::BlockChainServer::GetBlocks RawType;
    private:
        GetBlocksMessage(const Proto::BlockChainServer::GetBlocks& raw): ProtobufMessage(raw){}
        GetBlocksMessage(const uint256_t& start_hash, const uint256_t& stop_hash): ProtobufMessage(){
            raw_.set_head_hash(HexString(start_hash));
            raw_.set_stop_hash(HexString(stop_hash));
        }
    public:
        ~GetBlocksMessage(){}

        uint256_t GetHeadHash() const{
            return HashFromHexString(raw_.head_hash());
        }

        uint256_t GetStopHash() const{
            return HashFromHexString(raw_.stop_hash());
        }

        DECLARE_MESSAGE(GetBlocks);

        static GetBlocksMessage* NewInstance(const uint256_t& start_hash=BlockChain::GetHead().GetHash(), const uint256_t& stop_hash=uint256_t()){
            GetBlocksMessage* instance = (GetBlocksMessage*)Allocator::Allocate(sizeof(GetBlocksMessage), Object::kMessage);
            new (instance)GetBlocksMessage(start_hash, stop_hash);
            return instance;
        }

        static GetBlocksMessage* NewInstance(const RawType& raw){
            GetBlocksMessage* instance = (GetBlocksMessage*)Allocator::Allocate(sizeof(GetBlocksMessage), Object::kMessage);
            new (instance)GetBlocksMessage(raw);
            return instance;
        }
    };

    class NotFoundMessage : public ProtobufMessage<Proto::BlockChainServer::NotFound>{
    public:
        typedef Proto::BlockChainServer::NotFound RawType;
    private:
        NotFoundMessage(const RawType& raw): ProtobufMessage(raw){}
        NotFoundMessage(const InventoryItem& item, const std::string& message): ProtobufMessage(){
            raw_.set_message(message);
            (*raw_.mutable_item()) << item;
        }
    public:
        ~NotFoundMessage() = default;

        std::string GetMessage() const{
            return raw_.message();
        }

        InventoryItem GetItem() const{
            return InventoryItem(raw_.item());
        }

        DECLARE_MESSAGE(NotFound);

        static NotFoundMessage* NewInstance(const InventoryItem& item, const std::string& message="Not Found"){
            NotFoundMessage* instance = (NotFoundMessage*)Allocator::Allocate(sizeof(NotFoundMessage), Object::Type::kMessage);
            return new (instance)NotFoundMessage(item, message);
        }

        static NotFoundMessage* NewInstance(const RawType& raw){
            NotFoundMessage* instance = (NotFoundMessage*)Allocator::Allocate(sizeof(NotFoundMessage), Object::Type::kMessage);
            return new (instance)NotFoundMessage(raw);
        }
    };
}

#endif //TOKEN_MESSAGE_H