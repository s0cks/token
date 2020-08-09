#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "token.h"
#include "common.h"
#include "server.h"
#include "block_pool.h"
#include "block_chain.h"
#include "transaction_pool.h"
#include "unclaimed_transaction_pool.h"

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
    V(GetUnclaimedTransactions)

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
            std::stringstream ss;
            ss << GetName() << "Message(" << GetMessageSize() << " Bytes)";
            return ss.str();
        }

#define DECLARE_TYPECHECK(Name) \
    bool Is##Name##Message(){ return GetMessageType() == Message::k##Name##MessageType; }
        FOR_EACH_MESSAGE_TYPE(DECLARE_TYPECHECK)
#undef DECLARE_TYPECHECK

        static Handle<Message> Decode(MessageType type, uintptr_t size, uint8_t* bytes);
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

        size_t GetBufferSize() const{ return 0; }
        bool Encode(uint8_t* bytes) const{ return false; }
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

    //TODO:
    // - refactor this
    enum class ClientType{
        kNode,
        kClient
    };

    class VersionMessage : public ProtobufMessage<Proto::BlockChainServer::Version>{
    public:
        typedef Proto::BlockChainServer::Version RawType;
    private:
        VersionMessage(const RawType& raw): ProtobufMessage(raw){}
        VersionMessage(ClientType type, const std::string node_id, uint32_t timestamp, const std::string& nonce, const BlockHeader& head): ProtobufMessage(){
            raw_.set_node_id(node_id);
            raw_.set_version(Token::GetVersion());
            raw_.set_timestamp(timestamp);
            raw_.set_nonce(nonce);
            (*raw_.mutable_head()) << head;
            raw_.set_type(static_cast<uint32_t>(type));
        }
    public:
        ~VersionMessage(){}

        uint64_t GetTimestamp() const{
            return raw_.timestamp();
        }

        BlockHeader GetHead() const{
            return BlockHeader(raw_.head());
        }

        std::string GetVersion() const{
            return raw_.version();
        }

        std::string GetNonce() const{
            return raw_.nonce();
        }

        std::string GetNodeID() const{
            return raw_.node_id();
        }

        ClientType GetClientType() const{
            return static_cast<ClientType>(raw_.type());
        }

        bool IsNode() const{
            return GetClientType() == ClientType::kNode;
        }

        bool IsClient() const{
            return GetClientType() == ClientType::kClient;
        }

        DECLARE_MESSAGE(Version);

        static Handle<VersionMessage> NewInstance(ClientType type, const std::string& node_id, const std::string& nonce=GenerateNonce(), const BlockHeader& head=BlockChain::GetHead(), uint32_t timestamp=GetCurrentTime()){
            return new VersionMessage(type, node_id, timestamp, nonce, head);
        }

        static Handle<VersionMessage> NewInstance(const std::string& node_id){
            return NewInstance(ClientType::kClient, node_id, GenerateNonce(), BlockHeader());
        }

        static Handle<VersionMessage> NewInstance(const RawType& raw){
            return new VersionMessage(raw);
        }
    };

    class VerackMessage : public ProtobufMessage<Proto::BlockChainServer::Verack>{
    public:
        typedef Proto::BlockChainServer::Verack RawType;
    private:
        VerackMessage(const RawType& raw): ProtobufMessage(raw){}
        VerackMessage(ClientType type, const std::string& node_id, const std::string& nonce, const NodeAddress& address, const BlockHeader& head, uint32_t timestamp): ProtobufMessage(){
            raw_.set_node_id(node_id);
            raw_.set_version(Token::GetVersion());
            raw_.set_timestamp(timestamp);
            raw_.set_nonce(nonce);
            raw_.set_type(static_cast<uint32_t>(type));
            (*raw_.mutable_address()) << address;
            (*raw_.mutable_head()) << head;
        }
    public:
        ClientType GetClientType() const{
            return static_cast<ClientType>(raw_.type());
        }

        bool IsNode() const{
            return GetClientType() == ClientType::kNode;
        }

        bool IsClient() const{
            return GetClientType() == ClientType::kClient;
        }

        std::string GetID() const{
            return raw_.node_id();
        }

        NodeAddress GetCallbackAddress() const{
            return NodeAddress(raw_.address());
        }

        BlockHeader GetHead() const{
            return BlockHeader(raw_.head());
        }

        DECLARE_MESSAGE(Verack);

        static Handle<VerackMessage> NewInstance(ClientType type, const std::string& node_id, const NodeAddress& address, const BlockHeader& head=BlockChain::GetHead(), const std::string& nonce=GenerateNonce(), uint32_t timestamp=GetCurrentTime()){
            return new VerackMessage(type, node_id, nonce, address, head, timestamp);
        }

        static Handle<VerackMessage> NewInstance(const std::string& node_id){
            return NewInstance(ClientType::kClient, node_id, NodeAddress(), BlockHeader());
        }

        static Handle<VerackMessage> NewInstance(const RawType& raw){
            return new VerackMessage(raw);
        }
    };

    class Proposal;
    class PaxosMessage : public ProtobufMessage<Proto::BlockChainServer::Proposal>{
    protected:
        PaxosMessage(const Proto::BlockChainServer::Proposal& raw): ProtobufMessage(raw){}
        PaxosMessage(const std::string& node, Proposal* proposal);
    public:
        virtual ~PaxosMessage() = default;

        Handle<Proposal> GetProposal() const;

        //TODO:
        // - NodeInfo GetProposer()
        // - NodeInfo GetSubmitter()
        std::string GetProposer() const{
            return raw_.node_id();
        }

        uint256_t GetHash() const{
            return HashFromHexString(raw_.hash());
        }

        uint32_t GetHeight() const{
            return raw_.height();
        }
    };

    class PrepareMessage : public PaxosMessage{
    public:
        typedef Proto::BlockChainServer::Proposal RawType;
    private:
        PrepareMessage(const RawType& raw): PaxosMessage(raw){}
        PrepareMessage(const std::string& node_id, Proposal* proposal): PaxosMessage(node_id, proposal){}
    public:
        ~PrepareMessage(){}

        DECLARE_MESSAGE(Prepare);

        static Handle<PrepareMessage> NewInstance(Proposal* proposal, const std::string& node_id=Server::GetID()){
            return new PrepareMessage(node_id, proposal);
        }

        static Handle<PrepareMessage> NewInstance(const RawType& raw){
            return new PrepareMessage(raw);
        }
    };

    class PromiseMessage : public PaxosMessage{
    public:
        typedef Proto::BlockChainServer::Proposal RawType;
    private:
        PromiseMessage(const RawType& raw): PaxosMessage(raw){}
        PromiseMessage(const std::string& node_id, Proposal* proposal): PaxosMessage(node_id, proposal){}
    public:
        ~PromiseMessage(){}

        DECLARE_MESSAGE(Promise);

        static Handle<PromiseMessage> NewInstance(Proposal* proposal, const std::string& node_id=Server::GetID()){
            return new PromiseMessage(node_id, proposal);
        }

        static Handle<PromiseMessage> NewInstance(const RawType& raw){
            return new PromiseMessage(raw);
        }
    };

    class CommitMessage : public PaxosMessage{
    public:
        typedef Proto::BlockChainServer::Proposal RawType;
    private:
        CommitMessage(const RawType& raw): PaxosMessage(raw){}
        CommitMessage(const std::string& node_id, Proposal* proposal): PaxosMessage(node_id, proposal){}
    public:
        ~CommitMessage(){}

        DECLARE_MESSAGE(Commit);

        static Handle<CommitMessage> NewInstance(Proposal* proposal, const std::string& node_id=Server::GetID()){
            return new CommitMessage(node_id, proposal);
        }

        static Handle<CommitMessage> NewInstance(const RawType& raw){
            return new CommitMessage(raw);
        }
    };

    class AcceptedMessage : public PaxosMessage{
    public:
        typedef Proto::BlockChainServer::Proposal RawType;
    private:
        AcceptedMessage(const RawType& raw): PaxosMessage(raw){}
        AcceptedMessage(const std::string& node_id, Proposal* proposal): PaxosMessage(node_id, proposal){}
    public:
        ~AcceptedMessage(){}

        DECLARE_MESSAGE(Accepted);

        static Handle<AcceptedMessage> NewInstance(Proposal* proposal, const std::string& node_id=Server::GetID()){
            return new AcceptedMessage(node_id, proposal);
        }

        static Handle<AcceptedMessage> NewInstance(const RawType& raw){
            return new AcceptedMessage(raw);
        }
    };

    class RejectedMessage : public PaxosMessage{
    public:
        typedef Proto::BlockChainServer::Proposal RawType;
    private:
        RejectedMessage(const RawType& raw): PaxosMessage(raw){}
        RejectedMessage(const std::string& node_id, Proposal* proposal): PaxosMessage(node_id, proposal){}
    public:
        ~RejectedMessage(){}

        DECLARE_MESSAGE(Rejected);

        static Handle<RejectedMessage> NewInstance(Proposal* proposal, const std::string& node_id=Server::GetID()){
            return new RejectedMessage(node_id, proposal);
        }

        static Handle<RejectedMessage> NewInstance(const RawType& raw){
            return new RejectedMessage(raw);
        }
    };

    class TransactionMessage : public ProtobufMessage<Proto::BlockChain::Transaction>{
    public:
        typedef Proto::BlockChain::Transaction RawType;
    private:
        TransactionMessage(Transaction* tx): ProtobufMessage(){

        }

        TransactionMessage(const RawType& raw): ProtobufMessage(raw){}
    public:
        ~TransactionMessage(){}

        Transaction* GetTransaction() const{
            return nullptr;
        }

        DECLARE_MESSAGE(Transaction);

        static Handle<TransactionMessage> NewInstance(Transaction* tx){
            return new TransactionMessage(tx);
        }

        static Handle<TransactionMessage> NewInstance(const RawType& raw){
            return new TransactionMessage(raw);
        }
    };

    class BlockMessage : public ProtobufMessage<Proto::BlockChain::Block>{
    public:
        typedef Proto::BlockChain::Block RawType;
    private:
        BlockMessage(Block* blk): ProtobufMessage(){
            if(!blk->WriteToMessage(raw_)){
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

        static Handle<BlockMessage> NewInstance(Block* blk){
            return new BlockMessage(blk);
        }

        static Handle<BlockMessage> NewInstance(const RawType& raw){
            return new BlockMessage(raw);
        }
    };

    class InventoryItem{
    public:
        typedef Proto::BlockChainServer::InventoryItem RawType;

        enum Type{
            kUnknown=0,
            kTransaction,
            kBlock,
            kUnclaimedTransaction
        };
    private:
        Type type_;
        uint256_t hash_;
    public:
        InventoryItem():
            type_(kUnknown),
            hash_(){}
        InventoryItem(const RawType& raw):
            type_(static_cast<Type>(raw.type())),
            hash_(HashFromHexString(raw.hash())){}
        InventoryItem(Type type, const uint256_t& hash):
            type_(type),
            hash_(hash){}
        InventoryItem(const Handle<Transaction>& tx): InventoryItem(kTransaction, tx->GetHash()){}
        InventoryItem(const Handle<Block>& blk): InventoryItem(kBlock, blk->GetSHA256Hash()){}
        InventoryItem(const Handle<UnclaimedTransaction>& utxo): InventoryItem(kUnclaimedTransaction, utxo->GetHash()){}
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
                case kBlock: return BlockChain::HasBlock(hash_) || BlockPool::HasBlock(hash_);
                case kUnclaimedTransaction: UnclaimedTransactionPool::HasUnclaimedTransaction(hash_);
                default: return false;
            }
        }

        bool IsUnclaimedTransaction() const{
            return type_ == kUnclaimedTransaction;
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

        static const size_t kMaxAmountOfItemsPerMessage = 50;
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

        static Handle<InventoryMessage> NewInstance(std::vector<InventoryItem>& items){
            return new InventoryMessage(items);
        }

        static Handle<InventoryMessage> NewInstance(const Handle<Transaction>& tx){
            std::vector<InventoryItem> items = {
                InventoryItem(tx)
            };
            return NewInstance(items);
        }

        static Handle<InventoryMessage> NewInstance(const Handle<Block>& blk){
            std::vector<InventoryItem> items = {
                InventoryItem(blk)
            };
            return NewInstance(items);
        }

        static Handle<InventoryMessage> NewInstance(const RawType& raw){
            return new InventoryMessage(raw);
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

        static Handle<GetDataMessage> NewInstance(std::vector<InventoryItem>& items){
            return new GetDataMessage(items);
        }

        static Handle<GetDataMessage> NewInstance(const RawType& raw){
            return new GetDataMessage(raw);
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

        static Handle<GetBlocksMessage> NewInstance(const uint256_t& start_hash=BlockChain::GetHead().GetHash(), const uint256_t& stop_hash=uint256_t()){
            return new GetBlocksMessage(start_hash, stop_hash);
        }

        static Handle<GetBlocksMessage> NewInstance(const RawType& raw){
            return new GetBlocksMessage(raw);
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

        static Handle<NotFoundMessage> NewInstance(const InventoryItem& item, const std::string& message="Not Found"){
            return new NotFoundMessage(item, message);
        }

        static Handle<NotFoundMessage> NewInstance(const RawType& raw){
            return new NotFoundMessage(raw);
        }
    };

    //@SideClientServerOnly
    class GetUnclaimedTransactionsMessage : public Message{
    private:
        std::string user_;

        GetUnclaimedTransactionsMessage(const std::string& user):
            Message(),
            user_(user){}

        size_t GetBufferSize() const{ return 0; }
        bool Encode(uint8_t* bytes) const{ return false; }
    public:
        ~GetUnclaimedTransactionsMessage() = default;

        std::string GetUser() const{
            return user_;
        }

        uintptr_t GetMessageSize() const{
            return user_.size();
        }

        bool Encode(uint8_t* bytes, uintptr_t size) const{
            memcpy(bytes, user_.c_str(), size);
            return true;
        }

        DECLARE_MESSAGE(GetUnclaimedTransactions);

        static Handle<GetUnclaimedTransactionsMessage> NewInstance(const std::string& user){
            return new GetUnclaimedTransactionsMessage(user);
        }
    };
}

#endif //TOKEN_MESSAGE_H