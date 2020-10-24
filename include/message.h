#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include "uuid.h"
#include "token.h"
#include "common.h"
#include "version.h"
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
    V(UnclaimedTransaction) \
    V(Inventory) \
    V(NotFound) \
    V(GetUnclaimedTransactions)

#define FORWARD_DECLARE(Name) class Name##Message;
    FOR_EACH_MESSAGE_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

    //TODO: better encoding/decoding
    class Message : public Object{
        friend class Session;
    public:
        enum MessageType{
            kUnknownMessageType = 0,
#define DECLARE_MESSAGE_TYPE(Name) k##Name##MessageType,
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_TYPE)
#undef DECLARE_MESSAGE_TYPE
        };

        static const intptr_t kHeaderSize = sizeof(uint32_t) + sizeof(intptr_t);
    protected:
        Message() = default;

        virtual intptr_t GetMessageSize() const = 0;
        virtual bool WriteMessage(ByteBuffer* bytes) const = 0;
    public:
        virtual ~Message() = default;

        virtual const char* GetName() const{
            return "Unknown";
        }

        virtual MessageType GetMessageType() const{
            return MessageType::kUnknownMessageType;
        }

#define DECLARE_TYPECHECK(Name) \
    bool Is##Name##Message(){ return GetMessageType() == Message::k##Name##MessageType; }
        FOR_EACH_MESSAGE_TYPE(DECLARE_TYPECHECK)
#undef DECLARE_TYPECHECK
    };

#define DECLARE_MESSAGE(Name) \
    public: \
        virtual intptr_t GetMessageSize() const; \
        virtual bool WriteMessage(ByteBuffer* bytes) const; \
        virtual MessageType GetMessageType() const{ return Message::k##Name##MessageType; } \
        virtual const char* GetName() const{ return #Name; } \
        virtual std::string ToString() const{ \
            std::stringstream ss; \
            ss << #Name << "Message()"; \
            return ss.str(); \
        }

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
        Version version_;
        Hash nonce_;
        UUID node_id_;
        BlockHeader head_;

        VersionMessage(ClientType type, const Version& version, const UUID& node_id, Timestamp timestamp, const Hash& nonce, const BlockHeader& head):
            Message(),
            client_type_(type),
            version_(version),
            nonce_(nonce),
            node_id_(node_id),
            head_(head){}
    public:
        ~VersionMessage() = default;

        Timestamp GetTimestamp() const{
            return timestamp_;
        }

        BlockHeader GetHead() const{
            return head_;
        }

        Version GetVersion() const{
            return version_;
        }

        Hash GetNonce() const{
            return nonce_;
        }

        UUID GetID() const{
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

        DECLARE_MESSAGE(Version);

        static Handle<VersionMessage> NewInstance(ByteBuffer* bytes);
        static Handle<VersionMessage> NewInstance(ClientType type, const UUID& node_id, const Version& version=Version(), const Hash& nonce=Hash::GenerateNonce(), const BlockHeader& head=BlockChain::GetHead(), Timestamp timestamp=GetCurrentTimestamp()){
            return new VersionMessage(type, version, node_id, timestamp, nonce, head);
        }

        static Handle<VersionMessage> NewInstance(const UUID& node_id){
            return NewInstance(ClientType::kClient, node_id, Version(), Hash::GenerateNonce(), Block::Genesis()->GetHeader());
        }
    };

    class VerackMessage : public Message{
    private:
        Timestamp timestamp_;
        UUID node_id_;
        Version version_;
        Hash nonce_;
        ClientType client_type_;
        NodeAddress callback_;
        BlockHeader head_;

        VerackMessage(ClientType type, const UUID& node_id, const Version& version, const Hash& nonce, const NodeAddress& address, const BlockHeader& head, Timestamp timestamp):
            Message(),
            timestamp_(timestamp),
            node_id_(node_id),
            version_(version),
            nonce_(nonce),
            client_type_(type),
            callback_(address),
            head_(head){}
    public:
        Timestamp GetTimestamp() const{
            return timestamp_;
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

        UUID GetID() const{
            return node_id_;
        }

        NodeAddress GetCallbackAddress() const{
            return callback_;
        }

        BlockHeader GetHead() const{
            return head_;
        }

        DECLARE_MESSAGE(Verack);

        static Handle<VerackMessage> NewInstance(ByteBuffer* bytes);
        static Handle<VerackMessage> NewInstance(ClientType type, const UUID& node_id, const NodeAddress& address, const BlockHeader& head=BlockChain::GetHead(), const Version& version=Version(), const Hash& nonce=Hash::GenerateNonce(), Timestamp timestamp=GetCurrentTimestamp()){
            return new VerackMessage(type, node_id, version, nonce, address, head, timestamp);
        }

        static Handle<VerackMessage> NewInstance(const UUID& node_id){
            return NewInstance(ClientType::kClient, node_id, NodeAddress(), Block::Genesis()->GetHeader());
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

        intptr_t GetMessageSize() const;
        bool WriteMessage(ByteBuffer* bytes) const;
    public:
        virtual ~PaxosMessage() = default;

        MessageType GetMessageType() const{
            return type_;
        }

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

        DECLARE_MESSAGE(Block);

        static Handle<BlockMessage> NewInstance(ByteBuffer* bytes);
        static Handle<BlockMessage> NewInstance(Block* blk){
            return new BlockMessage(blk);
        }
    };

    class UnclaimedTransactionMessage : public Message {
    private:
        UnclaimedTransaction* data_;

        UnclaimedTransactionMessage(const Handle<UnclaimedTransaction>& utxo):
            Message(),
            data_(nullptr){
            WriteBarrier(&data_, utxo);
        }
    protected:
        bool Accept(WeakObjectPointerVisitor* vis){
            return vis->Visit(&data_);
        }
    public:
        ~UnclaimedTransactionMessage() = default;

        Handle<UnclaimedTransaction> GetUnclaimedTransaction() const{
            return data_;
        }

        DECLARE_MESSAGE(UnclaimedTransaction);

        static Handle<UnclaimedTransactionMessage> NewInstance(ByteBuffer* bytes);
        static Handle<UnclaimedTransactionMessage> NewInstance(const Handle<UnclaimedTransaction>& utxo){
            return new UnclaimedTransactionMessage(utxo);
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

        static const size_t kSize = sizeof(int32_t) + Hash::kSize;
    private:
        Type type_;
        Hash hash_;
    public:
        InventoryItem():
            type_(kUnknown),
            hash_(){}
        InventoryItem(Type type, const Hash& hash):
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

        Hash GetHash() const{
            return hash_;
        }

        bool ItemExists() const{
            switch(type_){
                case kTransaction: return TransactionPool::HasTransaction(hash_);
                case kBlock: return BlockChain::HasBlock(hash_) || BlockPool::HasBlock(hash_);
                case kUnclaimedTransaction: return UnclaimedTransactionPool::HasUnclaimedTransaction(hash_);
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
            } else if(item.IsUnclaimedTransaction()){
                stream << "UnclaimedTransaction(" << item.GetHash() << ")";
            } else{
                stream << "Unknown(" << item.GetHash() << ")";
            }
            return stream;
        }
    };

    class InventoryMessage : public Message{
    public:
        static const size_t kMaxAmountOfItemsPerMessage = 50;
    protected:
        std::vector<InventoryItem> items_;

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
            std::copy(items_.begin(), items_.end(), std::back_inserter(items));
            return items.size() == items_.size();
        }

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
        static const intptr_t kMaxNumberOfBlocks;
    private:
        Hash start_;
        Hash stop_;

        GetBlocksMessage(const Hash& start_hash, const Hash& stop_hash):
            Message(),
            start_(start_hash),
            stop_(stop_hash){}
    public:
        ~GetBlocksMessage(){}

        Hash GetHeadHash() const{
            return start_;
        }

        Hash GetStopHash() const{
            return stop_;
        }

        DECLARE_MESSAGE(GetBlocks);

        static Handle<GetBlocksMessage> NewInstance(ByteBuffer* bytes);
        static Handle<GetBlocksMessage> NewInstance(const Hash& start_hash=BlockChain::GetHead().GetHash(), const Hash& stop_hash=Hash()){
            return new GetBlocksMessage(start_hash, stop_hash);
        }
    };

    class NotFoundMessage : public Message{
    private:
        InventoryItem item_;
        std::string message_;

        NotFoundMessage(const std::string& message):
            Message(),
            message_(message){}
    public:
        ~NotFoundMessage() = default;

        std::string GetMessage() const{
            return message_;
        }

        DECLARE_MESSAGE(NotFound);

        static Handle<NotFoundMessage> NewInstance(ByteBuffer* bytes);
        static Handle<NotFoundMessage> NewInstance(const std::string& message="Not Found"){
            return new NotFoundMessage(message);
        }
    };

    class GetUnclaimedTransactionsMessage : public Message{
    private:
        User user_;

        GetUnclaimedTransactionsMessage(const User& user):
            Message(),
            user_(user){}
    public:
        ~GetUnclaimedTransactionsMessage() = default;

        User GetUser() const{
            return user_;
        }

        DECLARE_MESSAGE(GetUnclaimedTransactions);

        static Handle<GetUnclaimedTransactionsMessage> NewInstance(ByteBuffer* bytes);
        static Handle<GetUnclaimedTransactionsMessage> NewInstance(const User& user){
            return new GetUnclaimedTransactionsMessage(user);
        }
    };
};

#endif //TOKEN_MESSAGE_H