#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include <set>
#include "object.h"
#include "uuid.h"
#include "address.h"
#include "version.h"
#include "proposal.h"
#include "blockchain.h"
#include "configuration.h"

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
    V(GetPeers) \
    V(PeerList)

#define FORWARD_DECLARE(Name) class Name##Message;
  FOR_EACH_MESSAGE_TYPE(FORWARD_DECLARE)
#undef FORWARD_DECLARE

#define DEFINE_PTR(Name) \
    typedef std::shared_ptr<Name##Message> Name##MessagePtr;
  FOR_EACH_MESSAGE_TYPE(DEFINE_PTR)
#undef DEFINE_PTR

  class Message;
  typedef std::shared_ptr<Message> MessagePtr;

  class Message : public Object{
    friend class Session;
   public:
    enum MessageType{
      kUnknownMessageType = 0,
#define DECLARE_MESSAGE_TYPE(Name) k##Name##MessageType,
      FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_TYPE)
#undef DECLARE_MESSAGE_TYPE
    };

    static const int64_t kHeaderSize = sizeof(int32_t)
        + sizeof(int64_t);
   protected:
    Message() = default;

    virtual int64_t GetMessageSize() const = 0;
    virtual bool Encode(const BufferPtr &buff) const = 0;
   public:
    virtual ~Message() = default;

    virtual const char *GetName() const{
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

    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += kHeaderSize;
      size += GetMessageSize();
      return size;
    }

    bool Write(const BufferPtr &buff) const{
      buff->PutInt(static_cast<int32_t>(GetMessageType()));
      buff->PutLong(GetBufferSize());
      return true;
    }

#define DEFINE_CHECK(Name) \
    bool Is##Name##Message(){ return GetMessageType() == Message::k##Name##MessageType; }
    FOR_EACH_MESSAGE_TYPE(DEFINE_CHECK)
#undef DEFINE_CHECK
  };

#define DECLARE_MESSAGE_TYPE(Name) \
    virtual MessageType GetMessageType() const{ return Message::k##Name##MessageType; } \
    virtual const char* GetName() const{ return #Name; }

#define DECLARE_MESSAGE(Name) \
    DECLARE_MESSAGE_TYPE(Name) \
    virtual int64_t GetMessageSize() const; \
    virtual bool Encode(const BufferPtr& buff) const;

//TODO:
// - refactor this
  enum class ClientType{
    kUnknown = 0,
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
   public:
    VersionMessage(ClientType type,
                   const Version &version,
                   const UUID &node_id,
                   Timestamp timestamp,
                   const Hash &nonce,
                   const BlockHeader &head):
        Message(),
        timestamp_(timestamp),
        client_type_(type),
        version_(version),
        nonce_(nonce),
        node_id_(node_id),
        head_(head){}
    VersionMessage(const BufferPtr &buff):
        Message(),
        timestamp_(buff->GetLong()),
        client_type_(static_cast<ClientType>(buff->GetInt())),
        version_(buff),
        nonce_(buff->GetHash()),
        node_id_(buff),
        head_(buff){}
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

    static MessagePtr NewInstance(const BufferPtr &buff){
      return std::make_shared<VersionMessage>(buff);
    }

    static MessagePtr NewInstance(ClientType type,
                                  const UUID &node_id,
                                  const BlockHeader &head,
                                  const Version &version = Version(),
                                  const Hash &nonce = Hash::GenerateNonce(),
                                  Timestamp timestamp = GetCurrentTimestamp()){
      return std::make_shared<VersionMessage>(type, version, node_id, timestamp, nonce, head);
    }

    static MessagePtr NewInstance(const UUID &node_id){
      BlockPtr genesis = Block::Genesis();
      return NewInstance(ClientType::kClient, node_id, genesis->GetHeader(), Version(), Hash::GenerateNonce());
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
   public:
    VerackMessage(ClientType type,
                  const UUID &node_id,
                  const BlockHeader &head = BlockHeader(),
                  const Version &version = Version(),
                  const NodeAddress &callback = BlockChainConfiguration::GetServerCallbackAddress(),
                  const Hash &nonce = Hash::GenerateNonce(),
                  Timestamp timestamp = GetCurrentTimestamp()):
        Message(),
        timestamp_(timestamp),
        client_type_(type),
        version_(version),
        nonce_(nonce),
        node_id_(node_id),
        callback_(callback),
        head_(head){}
    VerackMessage(const BufferPtr &buff):
        Message(),
        timestamp_(buff->GetLong()),
        client_type_(static_cast<ClientType>(buff->GetInt())),
        version_(buff),
        nonce_(buff->GetHash()),
        node_id_(buff),
        callback_(buff),
        head_(buff){}
    ~VerackMessage() = default;

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

    static MessagePtr NewInstance(ClientType type,
                                  const UUID &node_id,
                                  const BlockHeader &head = BlockHeader(),
                                  const Version &version = Version(),
                                  const NodeAddress &callback = BlockChainConfiguration::GetServerCallbackAddress(),
                                  const Hash &nonce = Hash::GenerateNonce(),
                                  Timestamp timestamp = GetCurrentTimestamp()){
      return std::make_shared<VerackMessage>(type, node_id, head, version, callback, nonce, timestamp);
    }

    static MessagePtr NewInstance(const BufferPtr &buff){
      return std::make_shared<VerackMessage>(buff);
    }
  };

  class Proposal;
  class PaxosMessage : public Message{
   protected:
    MessageType type_; //TODO: remove this bs
    RawProposal raw_;

    PaxosMessage(MessageType type, ProposalPtr proposal):
        Message(),
        type_(type),
        raw_(proposal->GetRaw()){}
    PaxosMessage(MessageType type, const BufferPtr &buff):
        Message(),
        type_(type),
        raw_(buff){}

    int64_t GetMessageSize() const{
      return RawProposal::GetSize();
    }

    bool Encode(const BufferPtr &buff) const{
      return raw_.Encode(buff);
    }
   public:
    virtual ~PaxosMessage() = default;

    MessageType GetMessageType() const{
      return type_;
    }

    RawProposal GetRaw() const{
      return raw_;
    }

    ProposalPtr GetProposal() const;
  };

  class PrepareMessage : public PaxosMessage{
   public:
    PrepareMessage(ProposalPtr proposal): PaxosMessage(Message::kPrepareMessageType, proposal){}
    PrepareMessage(const BufferPtr &buff): PaxosMessage(Message::kPrepareMessageType, buff){}
    ~PrepareMessage(){}

    DECLARE_MESSAGE(Prepare);

    static MessagePtr NewInstance(const ProposalPtr &proposal){
      return std::make_shared<PrepareMessage>(proposal);
    }

    static MessagePtr NewInstance(const BufferPtr &buff){
      return std::make_shared<PrepareMessage>(buff);
    }
  };

  class PromiseMessage : public PaxosMessage{
   public:
    PromiseMessage(ProposalPtr proposal): PaxosMessage(Message::kPromiseMessageType, proposal){}
    PromiseMessage(const BufferPtr &buff): PaxosMessage(Message::kPromiseMessageType, buff){}
    ~PromiseMessage(){}

    DECLARE_MESSAGE(Promise);

    static MessagePtr NewInstance(const ProposalPtr &proposal){
      return std::make_shared<PromiseMessage>(proposal);
    }

    static MessagePtr NewInstance(const BufferPtr &buff){
      return std::make_shared<PromiseMessage>(buff);
    }
  };

  class CommitMessage : public PaxosMessage{
   public:
    CommitMessage(ProposalPtr proposal): PaxosMessage(Message::kCommitMessageType, proposal){}
    CommitMessage(const BufferPtr &buff): PaxosMessage(Message::kCommitMessageType, buff){}
    ~CommitMessage(){}

    DECLARE_MESSAGE(Commit);

    static MessagePtr NewInstance(const ProposalPtr &proposal){
      return std::make_shared<CommitMessage>(proposal);
    }

    static MessagePtr NewInstance(const BufferPtr &buff){
      return std::make_shared<CommitMessage>(buff);
    }
  };

  class AcceptedMessage : public PaxosMessage{
   public:
    AcceptedMessage(ProposalPtr proposal): PaxosMessage(Message::kAcceptedMessageType, proposal){}
    AcceptedMessage(const BufferPtr &buff): PaxosMessage(Message::kAcceptedMessageType, buff){}
    ~AcceptedMessage(){}

    DECLARE_MESSAGE(Accepted);

    static MessagePtr NewInstance(const ProposalPtr &proposal){
      return std::make_shared<AcceptedMessage>(proposal);
    }

    static MessagePtr NewInstance(const BufferPtr &buff){
      return std::make_shared<AcceptedMessage>(buff);
    }
  };

  class RejectedMessage : public PaxosMessage{
   public:
    RejectedMessage(ProposalPtr proposal): PaxosMessage(Message::kRejectedMessageType, proposal){}
    RejectedMessage(const BufferPtr &buff): PaxosMessage(Message::kRejectedMessageType, buff){}
    ~RejectedMessage(){}

    DECLARE_MESSAGE(Rejected);

    static MessagePtr NewInstance(const ProposalPtr &proposal){
      return std::make_shared<RejectedMessage>(proposal);
    }

    static MessagePtr NewInstance(const BufferPtr &buff){
      return std::make_shared<RejectedMessage>(buff);
    }
  };

  template<class T>
  class ObjectMessage : public Message{
   protected:
    typedef std::shared_ptr<T> ObjectPtr;

    ObjectPtr value_;

    ObjectMessage(const ObjectPtr &value):
        Message(),
        value_(value){}
    ObjectMessage(const BufferPtr &buff):
        Message(),
        value_(T::NewInstance(buff)){}
   public:
    virtual ~ObjectMessage() = default;

    ObjectPtr GetValue() const{
      return value_;
    }
  };

  class TransactionMessage : public ObjectMessage<Transaction>{
   protected:
    int64_t GetMessageSize() const{
      return GetValue()->GetBufferSize();
    }

    bool Encode(const BufferPtr &buff) const{
      return GetValue()->Write(buff);
    }
   public:
    TransactionMessage(const TransactionPtr &value):
        ObjectMessage(value){}
    TransactionMessage(const BufferPtr &buff):
        ObjectMessage(buff){}
    ~TransactionMessage() = default;

    std::string ToString() const{
      std::stringstream ss;
      ss << "TransactionMessage(" << GetValue()->GetHash() << ")";
      return ss.str();
    }

    DECLARE_MESSAGE_TYPE(Transaction);

    static MessagePtr NewInstance(const TransactionPtr &tx){
      return std::make_shared<TransactionMessage>(tx);
    }

    static MessagePtr NewInstance(const BufferPtr &buff){
      return std::make_shared<TransactionMessage>(buff);
    }
  };

  class BlockMessage : public ObjectMessage<Block>{
   protected:
    int64_t GetMessageSize() const{
      return GetValue()->GetBufferSize();
    }

    bool Encode(const BufferPtr &buff) const{
      return GetValue()->Write(buff);
    }
   public:
    BlockMessage(const BlockPtr &blk):
        ObjectMessage(blk){}
    BlockMessage(const BufferPtr &buff):
        ObjectMessage(buff){}
    ~BlockMessage() = default;

    std::string ToString() const{
      std::stringstream ss;
      ss << "BlockMessage(" << GetValue()->GetHash() << ")";
      return ss.str();
    }

    DECLARE_MESSAGE_TYPE(Block);

    static MessagePtr NewInstance(const BlockPtr &blk){
      return std::make_shared<BlockMessage>(blk);
    }

    static MessagePtr NewInstance(const BufferPtr &buff){
      return std::make_shared<BlockMessage>(buff);
    }
  };

  class UnclaimedTransactionMessage : public ObjectMessage<UnclaimedTransaction>{
   protected:
    int64_t GetMessageSize() const{
      return GetValue()->GetBufferSize();
    }

    bool Encode(const BufferPtr &buff) const{
      return GetValue()->Write(buff);
    }
   public:
    UnclaimedTransactionMessage(const UnclaimedTransactionPtr &utxo):
        ObjectMessage(utxo){}
    UnclaimedTransactionMessage(const BufferPtr &buff):
        ObjectMessage(buff){}
    ~UnclaimedTransactionMessage() = default;

    DECLARE_MESSAGE_TYPE(UnclaimedTransaction);

    static MessagePtr NewInstance(const UnclaimedTransactionPtr &utxo){
      return std::make_shared<UnclaimedTransactionMessage>(utxo);
    }

    static MessagePtr NewInstance(const BufferPtr &buff){
      return std::make_shared<UnclaimedTransactionMessage>(buff);
    }
  };

  class InventoryItem{
   public:
    enum Type{
      kUnknown = 0,
      kTransaction,
      kBlock,
      kUnclaimedTransaction
    };
   private:
    Type type_;
    Hash hash_;
   public:
    InventoryItem():
        type_(kUnknown),
        hash_(){}
    InventoryItem(Type type, const Hash &hash):
        type_(type),
        hash_(hash){}
    InventoryItem(const Transaction &tx):
        InventoryItem(kTransaction, tx.GetHash()){}
    InventoryItem(const BlockPtr &blk):
        InventoryItem(kBlock, blk->GetHash()){}
    InventoryItem(const UnclaimedTransactionPtr &utxo):
        InventoryItem(kUnclaimedTransaction, utxo->GetHash()){}
    InventoryItem(const BlockHeader &blk):
        InventoryItem(kBlock, blk.GetHash()){}
    InventoryItem(const InventoryItem &item):
        type_(item.type_),
        hash_(item.hash_){}
    InventoryItem(const BufferPtr &buff):
        type_(static_cast<Type>(buff->GetShort())),
        hash_(buff->GetHash()){}
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

    bool Encode(const BufferPtr &buff) const{
      buff->PutShort(static_cast<int16_t>(GetType()));
      buff->PutHash(GetHash());
      return true;
    }

    void operator=(const InventoryItem &other){
      type_ = other.type_;
      hash_ = other.hash_;
    }

    friend bool operator==(const InventoryItem &a, const InventoryItem &b){
      return a.type_ == b.type_ &&
          a.hash_ == b.hash_;
    }

    friend bool operator!=(const InventoryItem &a, const InventoryItem &b){
      return !operator==(a, b);
    }

    friend std::ostream &operator<<(std::ostream &stream, const InventoryItem &item){
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

    static inline int64_t
    GetSize(){
      int64_t size = 0;
      size += sizeof(int16_t);
      size += Hash::GetSize();
      return size;
    }
  };

  class InventoryMessage : public Message{
   public:
    static const size_t kMaxAmountOfItemsPerMessage = 50;
   protected:
    std::vector<InventoryItem> items_;
   public:
    InventoryMessage(const std::vector<InventoryItem> &items):
        Message(),
        items_(items){
      if(items_.empty())
        LOG(WARNING) << "inventory created w/ zero size";
    }
    InventoryMessage(const BufferPtr &buff):
        Message(),
        items_(){
      int64_t num_items = buff->GetLong();
      for(int64_t idx = 0; idx < num_items; idx++)
        items_.push_back(InventoryItem(buff));
    }
    ~InventoryMessage(){}

    int64_t GetNumberOfItems() const{
      return items_.size();
    }

    bool GetItems(std::vector<InventoryItem> &items){
      std::copy(items_.begin(), items_.end(), std::back_inserter(items));
      return items.size() == items_.size();
    }

    DECLARE_MESSAGE(Inventory);

    static MessagePtr NewInstance(const BufferPtr &buff);
    static MessagePtr NewInstance(std::vector<InventoryItem> &items){
      return std::make_shared<InventoryMessage>(items);
    }

    static MessagePtr NewInstance(const Transaction &tx){
      std::vector<InventoryItem> items = {
          InventoryItem(tx)
      };
      return NewInstance(items);
    }

    static MessagePtr NewInstance(const BlockPtr &blk){
      std::vector<InventoryItem> items = {
          InventoryItem(blk)
      };
      return NewInstance(items);
    }
  };

  class GetDataMessage : public Message{
   public:
    static const size_t kMaxAmountOfItemsPerMessage = 50;
   protected:
    std::vector<InventoryItem> items_;
   public:
    GetDataMessage(const std::vector<InventoryItem> &items):
        Message(),
        items_(items){
      if(items_.empty())
        LOG(WARNING) << "inventory created w/ zero size";
    }
    ~GetDataMessage() = default;

    int64_t GetNumberOfItems() const{
      return items_.size();
    }

    bool GetItems(std::vector<InventoryItem> &items){
      std::copy(items_.begin(), items_.end(), std::back_inserter(items));
      return items.size() == items_.size();
    }

    DECLARE_MESSAGE(GetData);

    static MessagePtr NewInstance(const BufferPtr &buff);
    static MessagePtr NewInstance(std::vector<InventoryItem> &items){
      return std::make_shared<GetDataMessage>(items);
    }

    static MessagePtr NewInstance(const Transaction &tx){
      std::vector<InventoryItem> items = {
          InventoryItem(tx)
      };
      return NewInstance(items);
    }

    static MessagePtr NewInstance(const BlockPtr &blk){
      std::vector<InventoryItem> items = {
          InventoryItem(blk)
      };
      return NewInstance(items);
    }
  };

  class GetBlocksMessage : public Message{
   public:
    static const intptr_t kMaxNumberOfBlocks;
   private:
    Hash start_;
    Hash stop_;
   public:
    GetBlocksMessage(const Hash &start_hash, const Hash &stop_hash):
        Message(),
        start_(start_hash),
        stop_(stop_hash){}
    ~GetBlocksMessage(){}

    Hash GetHeadHash() const{
      return start_;
    }

    Hash GetStopHash() const{
      return stop_;
    }

    DECLARE_MESSAGE(GetBlocks);

    static MessagePtr NewInstance(const BufferPtr &buff);
    static MessagePtr NewInstance(const Hash &start_hash = BlockChain::GetHead()->GetHash(),
                                  const Hash &stop_hash = Hash()){
      return std::make_shared<GetBlocksMessage>(start_hash, stop_hash);
    }
  };

  class NotFoundMessage : public Message{
   private:
    InventoryItem item_;
    std::string message_;
   public:
    NotFoundMessage(const std::string &message):
        Message(),
        message_(message){}
    ~NotFoundMessage() = default;

    std::string GetMessage() const{
      return message_;
    }

    DECLARE_MESSAGE(NotFound);

    static MessagePtr NewInstance(const BufferPtr &buff);
    static MessagePtr NewInstance(const std::string &message = "Not Found"){
      return std::make_shared<NotFoundMessage>(message);
    }
  };

  class GetPeersMessage : public Message{
   public:
    GetPeersMessage() = default;
    GetPeersMessage(const BufferPtr &buff):
        Message(){}
    ~GetPeersMessage() = default;

    DECLARE_MESSAGE(GetPeers);

    static MessagePtr NewInstance(const BufferPtr &buff){
      return std::make_shared<GetPeersMessage>(buff);
    }

    static MessagePtr NewInstance(){
      return std::make_shared<GetPeersMessage>();
    }
  };

  typedef std::set<NodeAddress> PeerList;

  class PeerListMessage : public Message{
   private:
    PeerList peers_;
   public:
    PeerListMessage(const PeerList &peers):
        Message(),
        peers_(peers){
      if(peers_.empty())
        LOG(WARNING) << "sending empty peer list";
    }
    ~PeerListMessage() = default;

    int64_t GetNumberOfPeers() const{
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

    static MessagePtr NewInstance(const BufferPtr &buff);
    static MessagePtr NewInstance(const PeerList &peers){
      return std::make_shared<PeerListMessage>(peers);
    }
  };
}

#endif //TOKEN_MESSAGE_H