#ifndef TOKEN_MESSAGE_H
#define TOKEN_MESSAGE_H

#include <set>
#include "object.h"
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
    V(Inventory) \
    V(NotFound) \
    V(GetPeers) \
    V(PeerList)

  class Message;
  typedef std::shared_ptr<Message> MessagePtr;

#define DEFINE_MESSAGE(Name) \
  class Name##Message;        \
  typedef std::shared_ptr<Name##Message> Name##MessagePtr;

  FOR_EACH_MESSAGE_TYPE(DEFINE_MESSAGE)
#undef DEFINE_MESSAGE

  class Message : public SerializableObject{
    friend class Session;
   public:
    enum MessageType{
      kUnknownMessageType = 0,
#define DECLARE_MESSAGE_TYPE(Name) k##Name##MessageType,
      FOR_EACH_MESSAGE_TYPE(DECLARE_MESSAGE_TYPE)
#undef DECLARE_MESSAGE_TYPE
    };

    friend std::ostream& operator<<(std::ostream& stream, const MessageType& type){
      switch(type){
#define DEFINE_TOSTRING(Name) \
        case MessageType::k##Name##MessageType: \
          return stream << #Name;
        FOR_EACH_MESSAGE_TYPE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:
          return stream << "Unknown";
      }
    }

    static const int64_t kHeaderSize = sizeof(int32_t) + sizeof(int64_t);
   protected:
    Message() = default;

    virtual int64_t GetMessageSize() const = 0;
    virtual bool Encode(const BufferPtr& buff) const = 0;
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

    int64_t GetBufferSize() const{
      int64_t size = 0;
      size += kHeaderSize;
      size += GetMessageSize();
      return size;
    }

    bool Write(const BufferPtr& buff) const{
      return buff->PutInt(static_cast<int32_t>(GetMessageType()))
          && buff->PutLong(GetMessageSize())
          && Encode(buff);
    }

    virtual bool Equals(const MessagePtr& msg) const = 0;

#define DEFINE_CHECK(Name) \
    bool Is##Name##Message(){ return GetMessageType() == Message::k##Name##MessageType; }
    FOR_EACH_MESSAGE_TYPE(DEFINE_CHECK)
#undef DEFINE_CHECK
  };

#define DEFINE_MESSAGE_TYPE(Name) \
    virtual MessageType GetMessageType() const{ return Message::k##Name##MessageType; } \
    virtual const char* GetName() const{ return #Name; }
#define DEFINE_MESSAGE(Name) \
    DEFINE_MESSAGE_TYPE(Name) \
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
      const Version& version,
      const UUID& node_id,
      Timestamp timestamp,
      const Hash& nonce,
      const BlockHeader& head):
      Message(),
      timestamp_(timestamp),
      client_type_(type),
      version_(version),
      nonce_(nonce),
      node_id_(node_id),
      head_(head){}
    VersionMessage(const BufferPtr& buff):
      Message(),
      timestamp_(buff->GetLong()),
      client_type_(static_cast<ClientType>(buff->GetInt())),
      version_(buff),
      nonce_(buff->GetHash()),
      node_id_(buff->GetUUID()),
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

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsVersionMessage()){
        LOG(WARNING) << "not a version message.";
        return false;
      }
      VersionMessagePtr msg = std::static_pointer_cast<VersionMessage>(obj);
      return timestamp_ == msg->timestamp_
             && client_type_ == msg->client_type_
             && version_ == msg->version_
             && nonce_ == msg->nonce_
             && node_id_ == msg->node_id_
             && head_ == msg->head_;
    }

    DEFINE_MESSAGE(Version);

    static MessagePtr NewInstance(const BufferPtr& buff){
      return std::make_shared<VersionMessage>(buff);
    }

    static MessagePtr NewInstance(ClientType type,
      const UUID& node_id,
      const BlockHeader& head,
      const Version& version = Version(),
      const Hash& nonce = Hash::GenerateNonce(),
      Timestamp timestamp = GetCurrentTimestamp()){
      return std::make_shared<VersionMessage>(type, version, node_id, timestamp, nonce, head);
    }

    static MessagePtr NewInstance(const UUID& node_id){
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
      const UUID& node_id,
      const NodeAddress& callback,
      const Version& version,
      const BlockHeader& head,
      const Hash& nonce,
      Timestamp timestamp = GetCurrentTimestamp()):
      Message(),
      timestamp_(timestamp),
      client_type_(type),
      version_(version),
      nonce_(nonce),
      node_id_(node_id),
      callback_(callback),
      head_(head){}
    VerackMessage(const BufferPtr& buff):
      Message(),
      timestamp_(buff->GetLong()),
      client_type_(static_cast<ClientType>(buff->GetInt())),
      version_(buff),
      nonce_(buff->GetHash()),
      node_id_(buff->GetUUID()),
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

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsVerackMessage()){
        return false;
      }
      VerackMessagePtr msg = std::static_pointer_cast<VerackMessage>(obj);
      return timestamp_ == msg->timestamp_
             && client_type_ == msg->client_type_
             && version_ == msg->version_
             && nonce_ == msg->nonce_
             && node_id_ == msg->node_id_
             && callback_ == msg->callback_
             && head_ == msg->head_;
    }

    DEFINE_MESSAGE(Verack);

    static MessagePtr NewInstance(ClientType type,
      const UUID& node_id,
      const NodeAddress& callback,
      const Version& version,
      const BlockHeader& head,
      const Hash& nonce,
      Timestamp timestamp = GetCurrentTimestamp()){
      return std::make_shared<VerackMessage>(type, node_id, callback, version, head, nonce, timestamp);
    }

    static MessagePtr NewInstance(const BufferPtr& buff){
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
    PaxosMessage(MessageType type, const BufferPtr& buff):
      Message(),
      type_(type),
      raw_(buff){}

    int64_t GetMessageSize() const{
      return RawProposal::GetSize();
    }

    bool Encode(const BufferPtr& buff) const{
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

    bool ProposalEquals(const std::shared_ptr<PaxosMessage>& msg) const{
      LOG(INFO) << raw_ << " <=> " <<msg->raw_;
      return raw_ == msg->raw_;
    }
  };

  class PrepareMessage : public PaxosMessage{
   public:
    PrepareMessage(const ProposalPtr& proposal):
      PaxosMessage(Message::kPrepareMessageType, proposal){}
    PrepareMessage(const BufferPtr& buff):
      PaxosMessage(Message::kPrepareMessageType, buff){}
    ~PrepareMessage(){}

    DEFINE_MESSAGE(Prepare);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsPrepareMessage())
        return false;
      return PaxosMessage::ProposalEquals(std::static_pointer_cast<PaxosMessage>(obj));
    }

    static MessagePtr NewInstance(const ProposalPtr& proposal){
      return std::make_shared<PrepareMessage>(proposal);
    }

    static MessagePtr NewInstance(const BufferPtr& buff){
      return std::make_shared<PrepareMessage>(buff);
    }
  };

  class PromiseMessage : public PaxosMessage{
   public:
    PromiseMessage(ProposalPtr proposal):
      PaxosMessage(Message::kPromiseMessageType, proposal){}
    PromiseMessage(const BufferPtr& buff):
      PaxosMessage(Message::kPromiseMessageType, buff){}
    ~PromiseMessage(){}

    DEFINE_MESSAGE(Promise);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsPromiseMessage()){
        return false;
      }
      return PaxosMessage::ProposalEquals(std::static_pointer_cast<PaxosMessage>(obj));
    }

    static MessagePtr NewInstance(const ProposalPtr& proposal){
      return std::make_shared<PromiseMessage>(proposal);
    }

    static MessagePtr NewInstance(const BufferPtr& buff){
      return std::make_shared<PromiseMessage>(buff);
    }
  };

  class CommitMessage : public PaxosMessage{
   public:
    CommitMessage(ProposalPtr proposal):
      PaxosMessage(Message::kCommitMessageType, proposal){}
    CommitMessage(const BufferPtr& buff):
      PaxosMessage(Message::kCommitMessageType, buff){}
    ~CommitMessage(){}

    DEFINE_MESSAGE(Commit);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsCommitMessage())
        return false;
      return PaxosMessage::ProposalEquals(std::static_pointer_cast<PaxosMessage>(obj));
    }

    static MessagePtr NewInstance(const ProposalPtr& proposal){
      return std::make_shared<CommitMessage>(proposal);
    }

    static MessagePtr NewInstance(const BufferPtr& buff){
      return std::make_shared<CommitMessage>(buff);
    }
  };

  class AcceptedMessage : public PaxosMessage{
   public:
    AcceptedMessage(ProposalPtr proposal):
      PaxosMessage(Message::kAcceptedMessageType, proposal){}
    AcceptedMessage(const BufferPtr& buff):
      PaxosMessage(Message::kAcceptedMessageType, buff){}
    ~AcceptedMessage(){}

    DEFINE_MESSAGE(Accepted);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsAcceptedMessage()){
        return false;
      }
      return PaxosMessage::ProposalEquals(std::static_pointer_cast<PaxosMessage>(obj));
    }

    static MessagePtr NewInstance(const ProposalPtr& proposal){
      return std::make_shared<AcceptedMessage>(proposal);
    }

    static MessagePtr NewInstance(const BufferPtr& buff){
      return std::make_shared<AcceptedMessage>(buff);
    }
  };

  class RejectedMessage : public PaxosMessage{
   public:
    RejectedMessage(ProposalPtr proposal):
      PaxosMessage(Message::kRejectedMessageType, proposal){}
    RejectedMessage(const BufferPtr& buff):
      PaxosMessage(Message::kRejectedMessageType, buff){}
    ~RejectedMessage(){}

    DEFINE_MESSAGE(Rejected);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsRejectedMessage()){
        return false;
      }
      return PaxosMessage::ProposalEquals(std::static_pointer_cast<PaxosMessage>(obj));
    }

    static MessagePtr NewInstance(const ProposalPtr& proposal){
      return std::make_shared<RejectedMessage>(proposal);
    }

    static MessagePtr NewInstance(const BufferPtr& buff){
      return std::make_shared<RejectedMessage>(buff);
    }
  };

  template<class T>
  class ObjectMessage : public Message{
   protected:
    typedef std::shared_ptr<T> ObjectPtr;

    ObjectPtr value_;

    ObjectMessage(const ObjectPtr& value):
      Message(),
      value_(value){}
    ObjectMessage(const BufferPtr& buff):
      Message(),
      value_(T::FromBytes(buff)){}
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

    bool Encode(const BufferPtr& buff) const{
      return GetValue()->Write(buff);
    }
   public:
    TransactionMessage(const TransactionPtr& value):
      ObjectMessage(value){}
    TransactionMessage(const BufferPtr& buff):
      ObjectMessage(buff){}
    ~TransactionMessage() = default;

    std::string ToString() const{
      std::stringstream ss;
      ss << "TransactionMessage(" << GetValue()->GetHash() << ")";
      return ss.str();
    }

    DEFINE_MESSAGE_TYPE(Transaction);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsTransactionMessage()){
        return false;
      }
      //TODO: implement
      return true;
    }

    static MessagePtr NewInstance(const TransactionPtr& tx){
      return std::make_shared<TransactionMessage>(tx);
    }

    static MessagePtr NewInstance(const BufferPtr& buff){
      return std::make_shared<TransactionMessage>(buff);
    }
  };

  class BlockMessage : public ObjectMessage<Block>{
   protected:
    int64_t GetMessageSize() const{
      return value_->GetBufferSize();
    }

    bool Encode(const BufferPtr& buff) const{
      return value_->Write(buff);
    }
   public:
    BlockMessage(const BlockPtr& blk):
      ObjectMessage(blk){}
    BlockMessage(const BufferPtr& buff):
      ObjectMessage(buff){}
    ~BlockMessage() = default;

    std::string ToString() const{
      std::stringstream ss;
      ss << "BlockMessage(" << value_->GetHash() << ")";
      return ss.str();
    }

    DEFINE_MESSAGE_TYPE(Block);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsBlockMessage()){
        return false;
      }
      BlockMessagePtr msg = std::static_pointer_cast<BlockMessage>(obj);
      return GetValue()->Equals(msg->GetValue());
    }

    static MessagePtr NewInstance(const BlockPtr& blk){
      return std::make_shared<BlockMessage>(blk);
    }

    static MessagePtr NewInstance(const BufferPtr& buff){
      return std::make_shared<BlockMessage>(buff);
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
    InventoryItem(Type type, const Hash& hash):
      type_(type),
      hash_(hash){}
    InventoryItem(const Transaction& tx):
      InventoryItem(kTransaction, tx.GetHash()){}
    InventoryItem(const BlockPtr& blk):
      InventoryItem(kBlock, blk->GetHash()){}
    InventoryItem(const UnclaimedTransactionPtr& utxo):
      InventoryItem(kUnclaimedTransaction, utxo->GetHash()){}
    InventoryItem(const BlockHeader& blk):
      InventoryItem(kBlock, blk.GetHash()){}
    InventoryItem(const InventoryItem& item):
      type_(item.type_),
      hash_(item.hash_){}
    InventoryItem(const BufferPtr& buff):
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

    bool Write(const BufferPtr& buff) const{
      buff->PutShort(static_cast<int16_t>(GetType()));
      buff->PutHash(GetHash());
      return true;
    }

    void operator=(const InventoryItem& other){
      type_ = other.type_;
      hash_ = other.hash_;
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
    InventoryMessage(const std::vector<InventoryItem>& items):
      Message(),
      items_(items){
      if(items_.empty())
        LOG(WARNING) << "inventory created w/ zero size";
    }
    InventoryMessage(const BufferPtr& buff):
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

    bool GetItems(std::vector<InventoryItem>& items){
      std::copy(items_.begin(), items_.end(), std::back_inserter(items));
      return items.size() == items_.size();
    }

    DEFINE_MESSAGE(Inventory);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsInventoryMessage()){
        return false;
      }
      InventoryMessagePtr msg = std::static_pointer_cast<InventoryMessage>(obj);
      return items_ == msg->items_;
    }

    static MessagePtr NewInstance(const Hash& hash, const InventoryItem::Type& type){
      std::vector<InventoryItem> items = {
        InventoryItem(type, hash),
      };
      return NewInstance(items);
    }

    static MessagePtr NewInstance(const BufferPtr& buff);
    static MessagePtr NewInstance(std::vector<InventoryItem>& items){
      return std::make_shared<InventoryMessage>(items);
    }

    static MessagePtr NewInstance(const Transaction& tx){
      return NewInstance(tx.GetHash(), InventoryItem::kTransaction);
    }

    static MessagePtr NewInstance(const BlockPtr& blk){
      return NewInstance(blk->GetHash(), InventoryItem::kBlock);
    }
  };

  class GetDataMessage : public Message{
   public:
    static const size_t kMaxAmountOfItemsPerMessage = 50;
   protected:
    std::vector<InventoryItem> items_;
   public:
    GetDataMessage(const std::vector<InventoryItem>& items):
      Message(),
      items_(items){
      if(items_.empty())
        LOG(WARNING) << "inventory created w/ zero size";
    }
    ~GetDataMessage() = default;

    int64_t GetNumberOfItems() const{
      return items_.size();
    }

    bool GetItems(std::vector<InventoryItem>& items){
      std::copy(items_.begin(), items_.end(), std::back_inserter(items));
      return items.size() == items_.size();
    }

    DEFINE_MESSAGE(GetData);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsGetDataMessage()){
        return false;
      }
      GetDataMessagePtr msg = std::static_pointer_cast<GetDataMessage>(obj);
      return items_ == msg->items_;
    }

    static MessagePtr NewInstance(const BufferPtr& buff);
    static MessagePtr NewInstance(std::vector<InventoryItem>& items){
      return std::make_shared<GetDataMessage>(items);
    }

    static MessagePtr NewInstance(const Transaction& tx){
      std::vector<InventoryItem> items = {
        InventoryItem(tx)
      };
      return NewInstance(items);
    }

    static MessagePtr NewInstance(const BlockPtr& blk){
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
    GetBlocksMessage(const Hash& start_hash, const Hash& stop_hash):
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

    std::string ToString() const{
      std::stringstream ss;
      ss << "GetBlocksMessage(" << start_ << ":" << stop_ << ")";
      return ss.str();
    }

    DEFINE_MESSAGE(GetBlocks);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsGetBlocksMessage()){
        return false;
      }
      GetBlocksMessagePtr msg = std::static_pointer_cast<GetBlocksMessage>(obj);
      return start_ == msg->start_
             && stop_ == msg->stop_;
    }

    static MessagePtr NewInstance(const BufferPtr& buff);
    static MessagePtr NewInstance(const Hash& start_hash = BlockChain::GetHead()->GetHash(),
      const Hash& stop_hash = Hash()){
      return std::make_shared<GetBlocksMessage>(start_hash, stop_hash);
    }
  };

  class NotFoundMessage : public Message{
   private:
    InventoryItem item_;
    std::string message_;
   public:
    NotFoundMessage(const std::string& message):
      Message(),
      message_(message){}
    ~NotFoundMessage() = default;

    std::string GetMessage() const{
      return message_;
    }

    DEFINE_MESSAGE(NotFound);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsNotFoundMessage()){
        return false;
      }
      NotFoundMessagePtr msg = std::static_pointer_cast<NotFoundMessage>(obj);
      return item_ == msg->item_;
    }

    static MessagePtr NewInstance(const BufferPtr& buff);
    static MessagePtr NewInstance(const std::string& message = "Not Found"){
      return std::make_shared<NotFoundMessage>(message);
    }
  };

  class GetPeersMessage : public Message{
   public:
    GetPeersMessage() = default;
    GetPeersMessage(const BufferPtr& buff):
      Message(){}
    ~GetPeersMessage() = default;

    DEFINE_MESSAGE(GetPeers);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsGetPeersMessage()){
        return false;
      }
      return true;
    }

    static MessagePtr NewInstance(const BufferPtr& buff){
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
    PeerListMessage(const PeerList& peers):
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

    DEFINE_MESSAGE(PeerList);

    bool Equals(const MessagePtr& obj) const{
      if(!obj->IsPeerListMessage()){
        return false;
      }
      PeerListMessagePtr msg = std::static_pointer_cast<PeerListMessage>(obj);
      return peers_ == msg->peers_;
    }

    static MessagePtr NewInstance(const BufferPtr& buff);
    static MessagePtr NewInstance(const PeerList& peers){
      return std::make_shared<PeerListMessage>(peers);
    }
  };

  typedef std::vector<MessagePtr> MessageList;

  class MessageBufferWriter{
   private:
    BufferPtr buff_;
    MessageList& messages_;
    MessageList::iterator next_;
    MessageList::iterator end_;
    int64_t offset_;
   public:
    MessageBufferWriter(const BufferPtr& buff, MessageList& messages):
      buff_(buff),
      messages_(messages),
      next_(messages.begin()),
      end_(messages.end()),
      offset_(0){}
    ~MessageBufferWriter() = default;

    MessageList& messages(){
      return messages_;
    }

    MessagePtr& Next(){
      return (*next_);
    }

    bool HasNext() const{
      return next_ != end_;
    }

    bool WriteNext(uv_buf_t* buff){
      MessagePtr& next = (*next_);
      int64_t total_size = next->GetBufferSize();
      if(!next->Write(buff_)){
        LOG(ERROR) << "cannot serialize " << next->GetName() << "(" << total_size << " bytes) to buffer.";
        return false;
      }

      buff->len = total_size;
      buff->base = &buff_->data()[offset_];

      offset_ += total_size;
      next_++;
      return true;
    }
  };

  class MessageBufferReader{
   private:
    BufferPtr buff_;
    int64_t length_;
    int64_t offset_;
   public:
    MessageBufferReader(const BufferPtr& buff, int64_t length):
      buff_(buff),
      length_(length),
      offset_(0){}
    ~MessageBufferReader() = default;

    int64_t GetBufferLength() const{
      return length_;
    }

    int64_t GetCurrentOffset() const{
      return offset_;
    }

    bool HasNext() const{
      return (GetCurrentOffset() + Message::kHeaderSize) < GetBufferLength();
    }

    MessagePtr Next(){
      Message::MessageType mtype = static_cast<Message::MessageType>(buff_->GetInt());
      int64_t msize = buff_->GetLong();

    #ifdef TOKEN_DEBUG
      LOG(INFO) << "decoding message type " << mtype << " (" << msize << " bytes)....";
    #endif//TOKEN_DEBUG

      switch(mtype){
#define DEFINE_DECODE(Name) \
          case Message::MessageType::k##Name##MessageType:{ \
              offset_ += msize;                             \
              return Name##Message::NewInstance(buff_);     \
          }
        FOR_EACH_MESSAGE_TYPE(DEFINE_DECODE)
#undef DEFINE_DECODE
        case Message::MessageType::kUnknownMessageType:
        default:LOG(ERROR) << "unknown message type " << mtype << " of size " << msize;
          return nullptr;
      }
    }
  };
};

#endif //TOKEN_MESSAGE_H