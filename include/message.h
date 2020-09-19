#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "token.h"
#include "common.h"
#include "address.h"
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
            kTypeLength = 1,// should this be 2?
            kSizeOffset = kTypeLength,
            kSizeLength = 4,
            kDataOffset = (kTypeLength + kSizeLength),
            kHeaderSize = kDataOffset,
        };
    protected:
        Message(){}
    public:
        virtual ~Message() = default;

        virtual const char* GetName() const{
            return "Unknown";
        }

        virtual MessageType GetMessageType() const{
            return MessageType::kUnknownMessageType;
        }

        virtual size_t GetBufferSize() const = 0;
        virtual bool Encode(ByteBuffer* bytes) const = 0;

        std::string ToString() const{
            std::stringstream ss;
            ss << GetName() << "Message(" << GetBufferSize() << " Bytes)";
            return ss.str();
        }

#define DECLARE_TYPECHECK(Name) \
    bool Is##Name##Message(){ return GetMessageType() == Message::k##Name##MessageType; }
        FOR_EACH_MESSAGE_TYPE(DECLARE_TYPECHECK)
#undef DECLARE_TYPECHECK

        static Handle<Message> Decode(MessageType type, ByteBuffer* bytes);
    };

#define DECLARE_MESSAGE(Name) \
    public: \
        virtual MessageType GetMessageType() const{ return Message::k##Name##MessageType; } \
        virtual const char* GetName() const{ return #Name; }

    //TODO:
    // - refactor this
    enum class ClientType{
        kUnknown=0,
        kNode,
        kClient
    };

    class VersionMessage : public Message{
    private:
        Timestamp timestamp_;
        ClientType client_type_; //TODO: refactor this field
        std::string version_;
        std::string nonce_;
        std::string node_id_;
        Handle<Block> head_; //TODO: convert to pointer, handle WeakReferenceVisitor

        VersionMessage(ClientType type, const std::string& version, const std::string& node_id, Timestamp timestamp, const std::string& nonce, const Handle<Block>& head):
            Message(),
            client_type_(type),
            version_(version),
            nonce_(nonce),
            node_id_(node_id),
            head_(head){}
    public:
        ~VersionMessage() = default;

        int64_t GetTimestamp() const{
            return timestamp_;
        }

        Handle<Block> GetHead() const{
            return head_;
        }

        std::string GetVersion() const{
            return version_;
        }

        std::string GetNonce() const{
            return nonce_;
        }

        std::string GetID() const{
            return node_id_;
        }

        ClientType GetClientType() const{
            return client_type_;
        }

        bool IsNode() const{
            return GetClientType() == ClientType::kNode;
        }

        bool IsClient() const{
            return GetClientType() == ClientType::kClient;
        }

        size_t GetBufferSize() const;
        bool Encode(ByteBuffer* bytes) const;
        DECLARE_MESSAGE(Version);

        static Handle<VersionMessage> NewInstance(ByteBuffer* bytes);
        static Handle<VersionMessage> NewInstance(ClientType type, const std::string& node_id, const std::string& version=Token::GetVersion(), const std::string& nonce=GenerateNonce(), const Handle<Block>& head=BlockChain::GetHead(), Timestamp timestamp=GetCurrentTimestamp()){
            return new VersionMessage(type, version, node_id, timestamp, nonce, head);
        }

        static Handle<VersionMessage> NewInstance(const std::string& node_id){
            return NewInstance(ClientType::kClient, node_id, Token::GetVersion(), GenerateNonce(), Block::Genesis());
        }
    };

    class VerackMessage : public Message{
    private:
        Timestamp timestamp_;
        std::string node_id_;
        std::string version_;
        std::string nonce_;
        ClientType client_type_;
        NodeAddress callback_;
        Handle<Block> head_; //TODO: convert to pointer, handle WeakReferenceVisitor

        VerackMessage(ClientType type, const std::string& node_id, const std::string& nonce, const NodeAddress& address, const Handle<Block>& head, Timestamp timestamp):
            Message(),
            client_type_(type),
            node_id_(node_id),
            nonce_(nonce),
            callback_(address),
            head_(head),
            timestamp_(timestamp){}
    public:
        ClientType GetClientType() const{
            return client_type_;
        }

        bool IsNode() const{
            return GetClientType() == ClientType::kNode;
        }

        bool IsClient() const{
            return GetClientType() == ClientType::kClient;
        }

        std::string GetID() const{
            return node_id_;
        }

        NodeAddress GetCallbackAddress() const{
            return callback_;
        }

        Handle<Block> GetHead() const{
            return head_;
        }

        size_t GetBufferSize() const;
        bool Encode(ByteBuffer* bytes) const;
        DECLARE_MESSAGE(Verack);

        static Handle<VerackMessage> NewInstance(ByteBuffer* bytes);
        static Handle<VerackMessage> NewInstance(ClientType type, const std::string& node_id, const NodeAddress& address, const Handle<Block>& head=BlockChain::GetHead(), const std::string& nonce=GenerateNonce(), Timestamp timestamp=GetCurrentTimestamp()){
            return new VerackMessage(type, node_id, nonce, address, head, timestamp);
        }

        static Handle<VerackMessage> NewInstance(const std::string& node_id){
            return NewInstance(ClientType::kClient, node_id, NodeAddress(), Block::Genesis());
        }
    };

    class Proposal;
    class PaxosMessage : public Message{
    protected:
        MessageType type_; //TODO: remove this bs
        std::string node_;
        Proposal* proposal_;

        PaxosMessage(MessageType type, const std::string& node, Proposal* proposal):
            Message(),
            type_(type),
            node_(node),
            proposal_(nullptr){
            WriteBarrier(&proposal_, proposal);
        }
    public:
        virtual ~PaxosMessage() = default;

        MessageType GetMessageType() const{
            return type_;
        }

        size_t GetBufferSize() const;
        bool Encode(ByteBuffer* bytes) const;
        Handle<Proposal> GetProposal() const;

        //TODO:
        // - NodeInfo GetProposer()
        // - NodeInfo GetSubmitter()
        std::string GetProposer() const{
            return node_;
        }
    };

    class PrepareMessage : public PaxosMessage{
    private:
        PrepareMessage(const std::string& node_id, Proposal* proposal): PaxosMessage(Message::kPrepareMessageType, node_id, proposal){}
    public:
        ~PrepareMessage(){}

        DECLARE_MESSAGE(Prepare);

        static Handle<PrepareMessage> NewInstance(ByteBuffer* bytes);
        static Handle<PrepareMessage> NewInstance(Proposal* proposal, const std::string& node_id){
            return new PrepareMessage(node_id, proposal);
        }
    };

    class PromiseMessage : public PaxosMessage{
    private:
        PromiseMessage(const std::string& node_id, Proposal* proposal): PaxosMessage(Message::kPromiseMessageType, node_id, proposal){}
    public:
        ~PromiseMessage(){}

        DECLARE_MESSAGE(Promise);

        static Handle<PromiseMessage> NewInstance(ByteBuffer* bytes);
        static Handle<PromiseMessage> NewInstance(Proposal* proposal, const std::string& node_id){
            return new PromiseMessage(node_id, proposal);
        }
    };

    class CommitMessage : public PaxosMessage{
    private:
        CommitMessage(const std::string& node_id, Proposal* proposal): PaxosMessage(Message::kCommitMessageType, node_id, proposal){}
    public:
        ~CommitMessage(){}

        DECLARE_MESSAGE(Commit);

        static Handle<CommitMessage> NewInstance(ByteBuffer* bytes);
        static Handle<CommitMessage> NewInstance(Proposal* proposal, const std::string& node_id){
            return new CommitMessage(node_id, proposal);
        }
    };

    class AcceptedMessage : public PaxosMessage{
    private:
        AcceptedMessage(const std::string& node_id, Proposal* proposal): PaxosMessage(Message::kAcceptedMessageType, node_id, proposal){}
    public:
        ~AcceptedMessage(){}

        DECLARE_MESSAGE(Accepted);

        static Handle<AcceptedMessage> NewInstance(ByteBuffer* bytes);
        static Handle<AcceptedMessage> NewInstance(Proposal* proposal, const std::string& node_id){
            return new AcceptedMessage(node_id, proposal);
        }
    };

    class RejectedMessage : public PaxosMessage{
    private:
        RejectedMessage(const std::string& node_id, Proposal* proposal): PaxosMessage(Message::kRejectedMessageType, node_id, proposal){}
    public:
        ~RejectedMessage(){}

        DECLARE_MESSAGE(Rejected);

        static Handle<RejectedMessage> NewInstance(ByteBuffer* bytes);
        static Handle<RejectedMessage> NewInstance(Proposal* proposal, const std::string& node_id){
            return new RejectedMessage(node_id, proposal);
        }
    };

    class TransactionMessage : public Message{
    private:
        Transaction* data_;

        TransactionMessage(const Handle<Transaction>& tx):
            Message(),
            data_(nullptr){
            WriteBarrier(&data_, tx);
        }
    protected:
        bool Accept(WeakObjectPointerVisitor* vis){
            return vis->Visit(&data_);
        }
    public:
        ~TransactionMessage(){}

        Handle<Transaction> GetTransaction() const{
            return data_;
        }

        size_t GetBufferSize() const;
        bool Encode(ByteBuffer* bytes) const;
        DECLARE_MESSAGE(Transaction);

        static Handle<TransactionMessage> NewInstance(ByteBuffer* bytes);
        static Handle<TransactionMessage> NewInstance(const Handle<Transaction>& tx){
            return new TransactionMessage(tx);
        }
    };

    class BlockMessage : public Message{
    private:
        Block* data_;

        BlockMessage(const Handle<Block>& blk):
            Message(),
            data_(nullptr){
            WriteBarrier(&data_, blk);
        }
    protected:
        bool Accept(WeakObjectPointerVisitor* vis){
            return vis->Visit(&data_);
        }
    public:
        ~BlockMessage(){}

        Handle<Block> GetBlock() const{
            return data_;
        }

        size_t GetBufferSize() const;
        bool Encode(ByteBuffer* bytes) const;
        DECLARE_MESSAGE(Block);

        static Handle<BlockMessage> NewInstance(ByteBuffer* bytes);
        static Handle<BlockMessage> NewInstance(Block* blk){
            return new BlockMessage(blk);
        }
    };

    class InventoryItem{
    public:
        enum Type{
            kUnknown=0,
            kTransaction,
            kBlock,
            kUnclaimedTransaction
        };

        static const size_t kBufferSize = sizeof(uint16_t) + uint256_t::kSize;
    private:
        Type type_;
        uint256_t hash_;
    public:
        InventoryItem():
            type_(kUnknown),
            hash_(){}
        InventoryItem(Type type, const uint256_t& hash):
            type_(type),
            hash_(hash){}
        InventoryItem(const Handle<Transaction>& tx): InventoryItem(kTransaction, tx->GetHash()){}
        InventoryItem(const Handle<Block>& blk): InventoryItem(kBlock, blk->GetHash()){}
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

    class InventoryMessage : public Message{
    public:
        static const size_t kMaxAmountOfItemsPerMessage = 50;
    private:
        std::vector<InventoryItem> items_;
    protected:
        InventoryMessage(const std::vector<InventoryItem>& items):
            Message(),
            items_(items){
            if(items_.empty())
                LOG(WARNING) << "inventory created w/ zero size";
        }

        static void DecodeItems(ByteBuffer* bytes, std::vector<InventoryItem>& items, uint32_t num_items);
    public:

        ~InventoryMessage(){}

        size_t GetNumberOfItems() const{
            return items_.size();
        }

        bool GetItems(std::vector<InventoryItem>& items){
            items.resize(items_.size());
            items.insert(items.end(), items_.begin(), items_.end());
            return items.size() > 0;
        }

        size_t GetBufferSize() const;
        bool Encode(ByteBuffer* bytes) const;
        DECLARE_MESSAGE(Inventory);

        static Handle<InventoryMessage> NewInstance(ByteBuffer* bytes);
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
    };

    class GetDataMessage : public InventoryMessage{
    private:
        GetDataMessage(const std::vector<InventoryItem>& items): InventoryMessage(items){}
    public:
        ~GetDataMessage(){}

        DECLARE_MESSAGE(GetData);

        static Handle<GetDataMessage> NewInstance(ByteBuffer* bytes);

        static Handle<GetDataMessage> NewInstance(std::vector<InventoryItem>& items){
            return new GetDataMessage(items);
        }
    };

    class GetBlocksMessage : public Message{
    public:
        static const size_t kMaxNumberOfBlocks;
    private:
        uint256_t start_;
        uint256_t stop_;

        GetBlocksMessage(const uint256_t& start_hash, const uint256_t& stop_hash):
            Message(),
            start_(start_hash),
            stop_(stop_hash){}
    public:
        ~GetBlocksMessage(){}

        uint256_t GetHeadHash() const{
            return start_;
        }

        uint256_t GetStopHash() const{
            return stop_;
        }

        size_t GetBufferSize() const{
            return uint256_t::kSize * 2;
        }

        bool Encode(ByteBuffer* bytes) const;
        DECLARE_MESSAGE(GetBlocks);

        static Handle<GetBlocksMessage> NewInstance(ByteBuffer* bytes);
        static Handle<GetBlocksMessage> NewInstance(const uint256_t& start_hash=BlockChain::GetHead()->GetHash(), const uint256_t& stop_hash=uint256_t()){
            return new GetBlocksMessage(start_hash, stop_hash);
        }
    };

    class NotFoundMessage : public Message{
    private:
        InventoryItem item_;
        std::string message_;

        NotFoundMessage(const InventoryItem& item, const std::string& message):
            Message(),
            item_(item),
            message_(message){}
    public:
        ~NotFoundMessage() = default;

        std::string GetMessage() const{
            return message_;
        }

        InventoryItem GetItem() const{
            return item_;
        }

        size_t GetBufferSize() const;
        bool Encode(ByteBuffer* bytes) const;
        DECLARE_MESSAGE(NotFound);

        static Handle<NotFoundMessage> NewInstance(ByteBuffer* bytes);
        static Handle<NotFoundMessage> NewInstance(const InventoryItem& item, const std::string& message="Not Found"){
            return new NotFoundMessage(item, message);
        }
    };

    //@SideClientServerOnly
    class GetUnclaimedTransactionsMessage : public Message{
    private:
        std::string user_;

        GetUnclaimedTransactionsMessage(const std::string& user):
            Message(),
            user_(user){}
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

        size_t GetBufferSize() const;
        bool Encode(ByteBuffer* bytes) const;
        DECLARE_MESSAGE(GetUnclaimedTransactions);

        static Handle<GetUnclaimedTransactionsMessage> NewInstance(ByteBuffer* bytes);
        static Handle<GetUnclaimedTransactionsMessage> NewInstance(const std::string& user){
            return new GetUnclaimedTransactionsMessage(user);
        }
    };
}

#endif //TOKEN_MESSAGE_H