#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include <set>
#include "object.h"
#include "uuid.h"
#include "peer.h"
#include "address.h"
#include "version.h"
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
    V(UnclaimedTransaction) \
    V(Inventory) \
    V(NotFound) \
    V(GetUnclaimedTransactions)  \
    V(GetPeers) \
    V(PeerList)

#define FORWARD_DECLARE(Name) class Name##Message;
    FOR_EACH_MESSAGE_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

    class Message : public Object{
        friend class Session;
    public:
        enum MessageType{
            kUnknownMessageType = 0,
#define DECLARE_MESSAGE_TYPE(Name) k##Name##MessageType,
    FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_TYPE)
#undef DECLARE_MESSAGE_TYPE
        };

        static const intptr_t kHeaderSize = sizeof(int32_t)
                                          + sizeof(int64_t);
    protected:
        Message():
            Object(Type::kMessageType){}

        virtual intptr_t GetMessageSize() const = 0;
        virtual bool Write(Buffer* buffer) const = 0;
    public:
        virtual ~Message() = default;

        virtual const char* GetName() const{
            return "Unknown";
        }

        virtual MessageType GetMessageType() const{
            return MessageType::kUnknownMessageType;
        }

        virtual std::string ToString() const{
            std::stringstream ss;
            ss << GetName() << "Message(" << GetMessageSize() << " Bytes)";
            return ss.str();
        }

#define DECLARE_TYPECHECK(Name) \
    bool Is##Name##Message(){ return GetMessageType() == Message::k##Name##MessageType; }
        FOR_EACH_MESSAGE_TYPE(DECLARE_TYPECHECK)
#undef DECLARE_TYPECHECK
    };

#define DECLARE_MESSAGE(Name) \
    public: \
        virtual intptr_t GetMessageSize() const; \
        virtual bool Write(Buffer* buff) const; \
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
        Version version_;
        Hash nonce_;
        UUID node_id_;
        BlockHeader head_;

        VersionMessage(ClientType type, const Version& version, const UUID& node_id, Timestamp timestamp, const Hash& nonce, const BlockHeader& head):
            Message(),
            timestamp_(timestamp),
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

        static VersionMessage* NewInstance(Buffer* buff);
        static VersionMessage* NewInstance(ClientType type, const UUID& node_id, const Version& version=Version(), const Hash& nonce=Hash::GenerateNonce(), const BlockHeader& head=BlockChain::GetHead()->GetHeader(), Timestamp timestamp=GetCurrentTimestamp()){
            return new VersionMessage(type, version, node_id, timestamp, nonce, head);
        }

        static VersionMessage* NewInstance(const UUID& node_id){
            return NewInstance(ClientType::kClient, node_id, Version(), Hash::GenerateNonce(), Block::Genesis()->GetHeader());
        }
    };

    class VerackMessage : public Message{
    private:
        Timestamp timestamp_;
        ClientType client_type_;
        Version version_;
        Hash nonce_;
        UUID node_id_;
        NodeAddress callback_;
        BlockHeader head_;

        VerackMessage(ClientType type, const UUID& node_id, const Version& version, const Hash& nonce, const NodeAddress& address, const BlockHeader& head, Timestamp timestamp):
            Message(),
            timestamp_(timestamp),
            client_type_(type),
            version_(version),
            nonce_(nonce),
            node_id_(node_id),
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

        static VerackMessage* NewInstance(Buffer* buff);
        static VerackMessage* NewInstance(ClientType type, const UUID& node_id, const NodeAddress& address, const BlockHeader& head=BlockChain::GetHead()->GetHeader(), const Version& version=Version(), const Hash& nonce=Hash::GenerateNonce(), Timestamp timestamp=GetCurrentTimestamp()){
            return new VerackMessage(type, node_id, version, nonce, address, head, timestamp);
        }

        static VerackMessage* NewInstance(const UUID& node_id){
            return NewInstance(ClientType::kClient, node_id, NodeAddress(), Block::Genesis()->GetHeader());
        }
    };

    class Proposal;
    class PaxosMessage : public Message{
    protected:
        MessageType type_; //TODO: remove this bs
        UUID proposer_;
        Proposal* proposal_;

        PaxosMessage(MessageType type, const UUID& proposer, Proposal* proposal):
            Message(),
            type_(type),
            proposer_(proposer),
            proposal_(proposal){}

        intptr_t GetMessageSize() const;
        bool Write(Buffer* buff) const;
    public:
        virtual ~PaxosMessage() = default;

        MessageType GetMessageType() const{
            return type_;
        }

        Proposal* GetProposal() const;

        UUID GetProposer() const{
            return proposer_;
        }
    };

    class PrepareMessage : public PaxosMessage{
    private:
        PrepareMessage(const UUID& proposer, Proposal* proposal):
            PaxosMessage(Message::kPrepareMessageType, proposer, proposal){}
    public:
        ~PrepareMessage(){}

        DECLARE_MESSAGE(Prepare);

        static PrepareMessage* NewInstance(Buffer* buff);
        static PrepareMessage* NewInstance(Proposal* proposal, const UUID& proposer){
            return new PrepareMessage(proposer, proposal);
        }
    };

    class PromiseMessage : public PaxosMessage{
    private:
        PromiseMessage(const UUID& proposer, Proposal* proposal):
            PaxosMessage(MessageType::kPromiseMessageType, proposer, proposal){}
    public:
        ~PromiseMessage(){}

        DECLARE_MESSAGE(Promise);

        static PromiseMessage* NewInstance(Buffer* buff);
        static PromiseMessage* NewInstance(Proposal* proposal, const UUID& proposer){
            return new PromiseMessage(proposer, proposal);
        }
    };

    class CommitMessage : public PaxosMessage{
    private:
        CommitMessage(const UUID& proposer, Proposal* proposal):
            PaxosMessage(Message::kCommitMessageType, proposer, proposal){}
    public:
        ~CommitMessage(){}

        DECLARE_MESSAGE(Commit);

        static CommitMessage* NewInstance(Buffer* buff);
        static CommitMessage* NewInstance(Proposal* proposal, const UUID& proposer){
            return new CommitMessage(proposer, proposal);
        }
    };

    class AcceptedMessage : public PaxosMessage{
    private:
        AcceptedMessage(const UUID& proposer, Proposal* proposal):
            PaxosMessage(Message::kAcceptedMessageType, proposer, proposal){}
    public:
        ~AcceptedMessage(){}

        DECLARE_MESSAGE(Accepted);

        static AcceptedMessage* NewInstance(Buffer* buff);
        static AcceptedMessage* NewInstance(Proposal* proposal, const UUID& proposer){
            return new AcceptedMessage(proposer, proposal);
        }
    };

    class RejectedMessage : public PaxosMessage{
    private:
        RejectedMessage(const UUID& proposer, Proposal* proposal):
            PaxosMessage(Message::kRejectedMessageType, proposer, proposal){}
    public:
        ~RejectedMessage(){}

        DECLARE_MESSAGE(Rejected);

        static RejectedMessage* NewInstance(Buffer* buff);
        static RejectedMessage* NewInstance(Proposal* proposal, const UUID& proposer){
            return new RejectedMessage(proposer, proposal);
        }
    };

    class TransactionMessage : public Message{
    private:
        Transaction* data_;

        TransactionMessage(Transaction* tx):
            Message(),
            data_(tx){}
    public:
        ~TransactionMessage(){}

        Transaction* GetTransaction() const{
            return data_;
        }

        DECLARE_MESSAGE(Transaction);

        static TransactionMessage* NewInstance(Buffer* buff);
        static TransactionMessage* NewInstance(Transaction* tx){
            return new TransactionMessage(tx);
        }
    };

    class BlockMessage : public Message{
    private:
        Block* data_;

        BlockMessage(Block* blk):
            Message(),
            data_(blk){}
    public:
        ~BlockMessage(){}

        Block* GetData() const{
            return data_;
        }

        std::string ToString() const;
        DECLARE_MESSAGE(Block);

        static BlockMessage* NewInstance(Buffer* buff);
        static BlockMessage* NewInstance(Block* blk){
            return new BlockMessage(blk);
        }
    };

    class UnclaimedTransactionMessage : public Message {
    private:
        UnclaimedTransaction* data_;

        UnclaimedTransactionMessage(UnclaimedTransaction* utxo):
            Message(),
            data_(utxo){}
    public:
        ~UnclaimedTransactionMessage() = default;

        UnclaimedTransaction* GetUnclaimedTransaction() const{
            return data_;
        }

        DECLARE_MESSAGE(UnclaimedTransaction);

        static UnclaimedTransactionMessage* NewInstance(Buffer* buff);
        static UnclaimedTransactionMessage* NewInstance(UnclaimedTransaction* utxo){
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
        InventoryItem(Transaction* tx): InventoryItem(kTransaction, tx->GetHash()){}
        InventoryItem(Block* blk): InventoryItem(kBlock, blk->GetHash()){}
        InventoryItem(UnclaimedTransaction* utxo): InventoryItem(kUnclaimedTransaction, utxo->GetHash()){}
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

        bool ItemExists() const;

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

        static void DecodeItems(Buffer* buff, std::vector<InventoryItem>& items, int32_t num_items);
    public:

        ~InventoryMessage(){}

        int32_t GetNumberOfItems() const{
            return items_.size();
        }

        bool GetItems(std::vector<InventoryItem>& items){
            std::copy(items_.begin(), items_.end(), std::back_inserter(items));
            return items.size() == items_.size();
        }

        DECLARE_MESSAGE(Inventory);

        static InventoryMessage* NewInstance(Buffer* buff);
        static InventoryMessage* NewInstance(std::vector<InventoryItem>& items){
            return new InventoryMessage(items);
        }

        static InventoryMessage* NewInstance(Transaction* tx){
            std::vector<InventoryItem> items = {
                InventoryItem(tx)
            };
            return NewInstance(items);
        }

        static InventoryMessage* NewInstance(Block* blk){
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

        static GetDataMessage* NewInstance(Buffer* buff);

        static GetDataMessage* NewInstance(std::vector<InventoryItem>& items){
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

        static GetBlocksMessage* NewInstance(Buffer* buff);
        static GetBlocksMessage* NewInstance(const Hash& start_hash=BlockChain::GetHead()->GetHash(), const Hash& stop_hash=Hash()){
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

        static NotFoundMessage* NewInstance(Buffer* buff);
        static NotFoundMessage* NewInstance(const std::string& message="Not Found"){
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

        static GetUnclaimedTransactionsMessage* NewInstance(Buffer* buff);
        static GetUnclaimedTransactionsMessage* NewInstance(const User& user){
            return new GetUnclaimedTransactionsMessage(user);
        }
    };

    class GetPeersMessage : public Message {
    private:
        GetPeersMessage() = default;
    public:
        ~GetPeersMessage() = default;

        DECLARE_MESSAGE(GetPeers);

        static GetPeersMessage* NewInstance(Buffer* buff){
            return new GetPeersMessage();
        }

        static GetPeersMessage* NewInstance(){
            return new GetPeersMessage();
        }
    };

    class PeerListMessage : public Message{
    private:
        PeerList peers_;

        PeerListMessage(const PeerList& peers):
            Message(),
            peers_(peers){
            if(peers_.empty())
                LOG(WARNING) << "sending empty peer list";
        }
    public:
        ~PeerListMessage() = default;

        int32_t GetNumberOfPeers() const{
            return peers_.size();
        }

        PeerList::iterator peers_begin(){
            return peers_.begin();
        }

        PeerList::const_iterator peers_begin() const{
            return peers_.begin();
        }

        PeerList::iterator peers_end(){
            return peers_.end();
        }

        PeerList::const_iterator peers_end() const{
            return peers_.end();
        }

        DECLARE_MESSAGE(PeerList);

        static PeerListMessage* NewInstance(Buffer* buff);
        static PeerListMessage* NewInstance(const PeerList& peers){
            return new PeerListMessage(peers);
        }
    };
}

#endif //TOKEN_MESSAGE_H