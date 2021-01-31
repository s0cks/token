#include "pool.h"
#include "server/message.h"
#include "consensus/proposal.h"
#include "unclaimed_transaction.h"

namespace Token{
  RpcMessagePtr RpcMessage::From(RpcSession* session, const BufferPtr& buffer){
    RpcMessage::MessageType mtype = static_cast<RpcMessage::MessageType>(buffer->GetInt());
    int64_t msize = buffer->GetLong();
    switch(mtype){
#define DEFINE_DECODE(Name) \
        case RpcMessage::k##Name##MessageType: \
            return Name##Message::NewInstance(buffer);
        FOR_EACH_MESSAGE_TYPE(DEFINE_DECODE)
#undef DEFINE_DECODE
      case RpcMessage::MessageType::kUnknownMessageType:
      default:
        LOG(ERROR) << "unknown message type " << mtype << " of size " << msize;
        return nullptr;
    }
  }

  int64_t VersionMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(Timestamp); // timestamp_
    size += sizeof(int32_t); // client_type_
    size += Version::kSize; // version_
    size += Hash::GetSize(); // nonce_
    size += UUID::GetSize(); // node_id_
    size += BlockHeader::GetSize(); // head_
    return size;
  }

  bool VersionMessage::Encode(const BufferPtr& buff) const{
    return buff->PutLong(ToUnixTimestamp(timestamp_))
        && buff->PutInt(static_cast<int32_t>(client_type_))
        && version_.Write(buff)
        && buff->PutHash(nonce_)
        && buff->PutUUID(node_id_)
        && head_.Write(buff);
  }

  int64_t VerackMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(Timestamp); // timestamp_
    size += sizeof(int32_t); // client_type_
    size += Version::kSize; // version_
    size += Hash::GetSize(); // nonce_
    size += UUID::GetSize(); // node_id_
    size += NodeAddress::kSize; // callback_
    size += BlockHeader::GetSize(); // head_
    return size;
  }

  bool VerackMessage::Encode(const BufferPtr& buff) const{
    return buff->PutLong(ToUnixTimestamp(timestamp_))
        && buff->PutInt(static_cast<int32_t>(GetClientType()))
        && version_.Write(buff)
        && buff->PutHash(nonce_)
        && buff->PutUUID(node_id_)
        && callback_.Write(buff)
        && head_.Write(buff);
  }

  bool InventoryItem::ItemExists() const{
    switch(type_){
      case kTransaction: return ObjectPool::HasTransaction(hash_);
      case kBlock: return BlockChain::HasBlock(hash_) || ObjectPool::HasBlock(hash_);
      case kUnclaimedTransaction: return ObjectPool::HasUnclaimedTransaction(hash_);
      default: return false;
    }
  }

  RpcMessagePtr InventoryMessage::NewInstance(const BufferPtr& buff){
    int64_t num_items = buff->GetLong();
    std::vector<InventoryItem> items;
    for(int64_t idx = 0; idx < num_items; idx++)
      items.push_back(InventoryItem(buff));
    return std::make_shared<InventoryMessage>(items);
  }

  int64_t InventoryMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(int64_t); // length(items_)
    size += (items_.size() * InventoryItem::GetSize());
    return size;
  }

  bool InventoryMessage::Encode(const BufferPtr& buff) const{
    buff->PutList(items_);
    return true;
  }

  bool GetDataMessage::Encode(const BufferPtr& buff) const{
    buff->PutList(items_);
    return true;
  }

  RpcMessagePtr GetDataMessage::NewInstance(const BufferPtr& buff){
    int64_t num_items = buff->GetLong();
    std::vector<InventoryItem> items;
    for(int64_t idx = 0; idx < num_items; idx++)
      items.push_back(InventoryItem(buff));
    return std::make_shared<GetDataMessage>(items);
  }

  int64_t GetDataMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(int64_t); // sizeof(items_);
    size += items_.size() * InventoryItem::GetSize(); // items;
    return size;
  }

  const int64_t GetBlocksMessage::kMaxNumberOfBlocks = 32;

  RpcMessagePtr GetBlocksMessage::NewInstance(const BufferPtr& buff){
    Hash start = buff->GetHash();
    Hash stop = buff->GetHash();
    return std::make_shared<GetBlocksMessage>(start, stop);
  }

  bool GetBlocksMessage::Encode(const BufferPtr& buff) const{
    buff->PutHash(start_);
    buff->PutHash(stop_);
    return true;
  }

  RpcMessagePtr NotFoundMessage::NewInstance(const BufferPtr& buff){
    std::string message = ""; //TODO: buff->GetString();
    return std::make_shared<NotFoundMessage>(message);
  }

  int64_t NotFoundMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(int32_t);
    size += Hash::kSize;
    size += message_.length();
    return size;
  }

  bool NotFoundMessage::Encode(const BufferPtr& buff) const{
    buff->PutInt(static_cast<int32_t>(item_.GetType()));
    buff->PutHash(item_.GetHash());
    buff->PutString(message_);
    return true;
  }

  ProposalPtr PaxosMessage::GetProposal() const{
    return std::make_shared<Proposal>(raw_);
  }

  int64_t PrepareMessage::GetMessageSize() const{
    return PaxosMessage::GetMessageSize();
  }

  bool PrepareMessage::Encode(const BufferPtr& buff) const{
    return PaxosMessage::Encode(buff);
  }

  int64_t PromiseMessage::GetMessageSize() const{
    return PaxosMessage::GetMessageSize();
  }

  bool PromiseMessage::Encode(const BufferPtr& buff) const{
    return PaxosMessage::Encode(buff);
  }

  int64_t CommitMessage::GetMessageSize() const{
    return PaxosMessage::GetMessageSize();
  }

  bool CommitMessage::Encode(const BufferPtr& buff) const{
    return PaxosMessage::Encode(buff);
  }

  int64_t AcceptedMessage::GetMessageSize() const{
    return PaxosMessage::GetMessageSize();
  }

  bool AcceptedMessage::Encode(const BufferPtr& buff) const{
    return PaxosMessage::Encode(buff);
  }

  int64_t RejectedMessage::GetMessageSize() const{
    return PaxosMessage::GetMessageSize();
  }

  bool RejectedMessage::Encode(const BufferPtr& buff) const{
    return PaxosMessage::Encode(buff);
  }

  int64_t GetBlocksMessage::GetMessageSize() const{
    return Hash::GetSize() * 2;
  }

  RpcMessagePtr PeerListMessage::NewInstance(const BufferPtr& buff){
    PeerList peers;

    int64_t npeers = buff->GetLong();
    for(int64_t idx = 0; idx < npeers; idx++){
      NodeAddress peer(buff);
      if(!peers.insert(peer).second){
        LOG(WARNING) << "couldn't insert peer: " << peer;
        return nullptr;
      }
    }

    return std::make_shared<PeerListMessage>(peers);
  }

  int64_t PeerListMessage::GetMessageSize() const{
    int64_t size = 0;
    size += sizeof(int64_t);
    size += (GetNumberOfPeers() * NodeAddress::kSize);
    return size;
  }

  bool PeerListMessage::Encode(const BufferPtr& buff) const{
    buff->PutLong(GetNumberOfPeers());
    for(auto it = peers_begin();
      it != peers_end();
      it++){
      it->Write(buff);
    }
    return true;
  }

  int64_t GetPeersMessage::GetMessageSize() const{
    //TODO: implement GetPeersMessage::GetMessageSize()
    return 0;
  }

  bool GetPeersMessage::Encode(const BufferPtr& buff) const{
    //TODO: implement GetPeersMessage::Encode(ByteBuffer*)
    return true;
  }
}