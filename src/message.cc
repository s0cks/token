#include "message.h"
#include "proposal.h"

#include "byte_buffer.h"

namespace Token{
    Handle<VersionMessage> VersionMessage::NewInstance(ByteBuffer* bytes){
        uint64_t timestamp = bytes->GetLong();
        ClientType client_type = static_cast<ClientType>(bytes->GetShort());
        std::string version = bytes->GetString();
        std::string nonce = bytes->GetString();
        std::string node_id = bytes->GetString();
        BlockHeader head = BlockHeader(bytes);
        return new VersionMessage(client_type, version, node_id, timestamp, nonce, head);
    }

    size_t VersionMessage::GetBufferSize() const{
        size_t size = 0;
        size += sizeof(uint64_t); // timestamp_
        size += sizeof(uint16_t); // client_type_
        size += sizeof(uint32_t); // length(version_)
        size += version_.length(); // version_
        size += sizeof(uint32_t); // length(nonce_) TODO: don't encode string values
        size += nonce_.length(); // nonce_
        size += sizeof(uint32_t); // length(node_id_) TODO: don't encode string values
        size += node_id_.length(); // node_id_
        size += BlockHeader::kSize;
        return size;
    }

    bool VersionMessage::Encode(ByteBuffer* bytes) const{
        bytes->PutLong(timestamp_);
        bytes->PutShort(static_cast<uint16_t>(client_type_));
        bytes->PutString(version_);
        bytes->PutString(nonce_);
        bytes->PutString(node_id_);
        head_.Encode(bytes);
        return true;
    }

    Handle<VerackMessage> VerackMessage::NewInstance(ByteBuffer* bytes){
        uint64_t timestamp = bytes->GetLong();
        std::string node_id = bytes->GetString();
        std::string version = bytes->GetString();
        std::string nonce = bytes->GetString();
        ClientType client_type = static_cast<ClientType>(bytes->GetShort());
        NodeAddress address; //TODO: decode callback_
        BlockHeader head = BlockHeader(bytes);
        return new VerackMessage(client_type, node_id, version, nonce, address, head, timestamp);
    }

    size_t VerackMessage::GetBufferSize() const{
        size_t size = 0;
        size += sizeof(uint64_t); // timestamp_
        size += sizeof(uint32_t); // length(node_id_)
        size += node_id_.length(); // node_id_
        size += sizeof(uint32_t); // length(version_)
        size += version_.length(); // version_
        size += sizeof(uint32_t); // length(nonce_)
        size += nonce_.length(); // nonce_
        //TODO: calculate sizeof(callback_)
        size += BlockHeader::kSize;
        return size;
    }

    bool VerackMessage::Encode(ByteBuffer* bytes) const{
        bytes->PutLong(timestamp_);
        bytes->PutString(node_id_);
        bytes->PutString(version_);
        bytes->PutString(nonce_);
        //TODO: encode(callback_)
        head_.Encode(bytes);
        return true;
    }

    size_t PaxosMessage::GetBufferSize() const{
        size_t size = 0;
        size += (sizeof(uint32_t) + node_.length()); // node_
        size += proposal_->GetBufferSize();
        return size;
    }

    bool PaxosMessage::Encode(ByteBuffer* bytes) const{
        bytes->PutString(node_);
        proposal_->Encode(bytes);
        return true;
    }

    Handle<PrepareMessage> PrepareMessage::NewInstance(ByteBuffer* bytes){
        std::string node_id = bytes->GetString();
        Handle<Proposal> proposal = Proposal::NewInstance(bytes);
        return new PrepareMessage(node_id, proposal);
    }

    Handle<PromiseMessage> PromiseMessage::NewInstance(ByteBuffer* bytes){
        std::string node_id = bytes->GetString();
        Handle<Proposal> proposal = Proposal::NewInstance(bytes);
        return new PromiseMessage(node_id, proposal);
    }

    Handle<CommitMessage> CommitMessage::NewInstance(ByteBuffer* bytes){
        std::string node_id = bytes->GetString();
        Handle<Proposal> proposal = Proposal::NewInstance(bytes);
        return new CommitMessage(node_id, proposal);
    }

    Handle<AcceptedMessage> AcceptedMessage::NewInstance(ByteBuffer* bytes){
        std::string node_id = bytes->GetString();
        Handle<Proposal> proposal = Proposal::NewInstance(bytes);
        return new AcceptedMessage(node_id, proposal);
    }

    Handle<RejectedMessage> RejectedMessage::NewInstance(ByteBuffer* bytes){
        std::string node_id = bytes->GetString();
        Handle<Proposal> proposal = Proposal::NewInstance(bytes);
        return new RejectedMessage(node_id, proposal);
    }

    Handle<TransactionMessage> TransactionMessage::NewInstance(ByteBuffer* bytes){
        Handle<Transaction> tx = Transaction::NewInstance(bytes);
        return new TransactionMessage(tx);
    }

    size_t TransactionMessage::GetBufferSize() const{
        return data_->GetBufferSize();
    }

    bool TransactionMessage::Encode(ByteBuffer* bytes) const{
        return data_->Encode(bytes);
    }

    Handle<BlockMessage> BlockMessage::NewInstance(ByteBuffer* bytes){
        Handle<Block> blk = Block::NewInstance(bytes);
        return new BlockMessage(blk);
    }

    size_t BlockMessage::GetBufferSize() const{
        return data_->GetBufferSize();
    }

    bool BlockMessage::Encode(ByteBuffer* bytes) const{
        return data_->Encode(bytes);
    }

    Handle<InventoryMessage> InventoryMessage::NewInstance(ByteBuffer* bytes){
        uint32_t num_items = bytes->GetInt();
        std::vector<InventoryItem> items;
        DecodeItems(bytes, items, num_items);
        return new InventoryMessage(items);
    }

    void InventoryMessage::DecodeItems(Token::ByteBuffer* bytes, std::vector<InventoryItem>& items, uint32_t num_items){
        for(uint32_t idx = 0; idx < num_items; idx++){
            InventoryItem::Type type = static_cast<InventoryItem::Type>(bytes->GetShort());
            uint256_t hash = bytes->GetHash();
            items.push_back(InventoryItem(type, hash));
        }
    }

    size_t InventoryMessage::GetBufferSize() const{
        size_t size = 0;
        size += sizeof(uint32_t); // length(items_)
        size += (items_.size() * InventoryItem::kBufferSize);
        return size;
    }

    bool InventoryMessage::Encode(ByteBuffer* bytes) const{
        bytes->PutInt(items_.size());
        for(auto& it : items_){
            bytes->PutShort(static_cast<uint16_t>(it.GetType()));
            bytes->PutHash(it.GetHash());
        }
        return true;
    }

    Handle<GetDataMessage> GetDataMessage::NewInstance(ByteBuffer* bytes){
        uint32_t num_items = bytes->GetInt();
        std::vector<InventoryItem> items;
        DecodeItems(bytes, items, num_items);
        return new GetDataMessage(items);
    }

    const intptr_t GetBlocksMessage::kMaxNumberOfBlocks = 32;

    Handle<GetBlocksMessage> GetBlocksMessage::NewInstance(ByteBuffer* bytes){
        uint256_t start = bytes->GetHash();
        uint256_t stop = bytes->GetHash();
        return new GetBlocksMessage(start, stop);
    }

    bool GetBlocksMessage::Encode(ByteBuffer* bytes) const{
        bytes->PutHash(start_);
        bytes->PutHash(stop_);
        return true;
    }

    Handle<NotFoundMessage> NotFoundMessage::NewInstance(ByteBuffer* bytes){
        InventoryItem::Type item_type = static_cast<InventoryItem::Type>(bytes->GetShort());
        uint256_t item_hash = bytes->GetHash();
        std::string message = bytes->GetString();
        return new NotFoundMessage(InventoryItem(item_type, item_hash), message);
    }

    size_t NotFoundMessage::GetBufferSize() const{
        size_t size = 0;
        size += InventoryItem::kBufferSize; // item_
        size += sizeof(uint32_t); // length(message_)
        size += message_.length(); // message_
        return size;
    }

    bool NotFoundMessage::Encode(ByteBuffer* bytes) const{
        bytes->PutShort(static_cast<uint16_t>(item_.GetType()));
        bytes->PutHash(item_.GetHash());
        bytes->PutString(message_);
        return true;
    }

    Handle<GetUnclaimedTransactionsMessage> GetUnclaimedTransactionsMessage::NewInstance(ByteBuffer* bytes){
        std::string user = bytes->GetString();
        return new GetUnclaimedTransactionsMessage(user);
    }

    size_t GetUnclaimedTransactionsMessage::GetBufferSize() const{
        size_t size = 0;
        size += sizeof(uint32_t); // length(user_)
        size += user_.length();
        return size;
    }

    bool GetUnclaimedTransactionsMessage::Encode(ByteBuffer* bytes) const{
        bytes->PutString(user_);
        return true;
    }

    Handle<Message> Message::Decode(Message::MessageType type, ByteBuffer* bytes){
#define DECLARE_DECODE(Name) \
        case Message::MessageType::k##Name##MessageType:{ \
            return Name##Message::NewInstance(bytes).CastTo<Message>(); \
        }
        switch(type){
            FOR_EACH_MESSAGE_TYPE(DECLARE_DECODE)
            default:{
                LOG(ERROR) << "invalid message of type " << static_cast<uint16_t>(type);
                return nullptr;
            }
        }
#undef DECLARE_DECODE
    }

    Handle<Proposal> PaxosMessage::GetProposal() const{
        /*
        if(ProposerThread::HasProposal()){
            Handle<Proposal> proposal = ProposerThread::GetProposal();
            if(proposal->GetHeight() == GetHeight() &&
                proposal->GetHash() == GetHash()){
                return proposal;
            }

            LOG(WARNING) << "current proposal #" << proposal->GetHeight() << "(" << proposal->GetHash() << ") is invalid";
            LOG(WARNING) << "expected proposal #" << GetHeight() << "(" << GetHash() << ")";
        }
        */
        return proposal_;
    }
}