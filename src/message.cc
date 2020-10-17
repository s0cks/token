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

    intptr_t VersionMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(Timestamp); // timestamp_
        size += sizeof(uint32_t); // client_type_
        // TODO: don't encode string values
        size += sizeof(uint32_t) + version_.length(); // version_
        size += sizeof(uint32_t) + nonce_.length(); // nonce_
        size += sizeof(uint32_t) + node_id_.length(); // node_id
        size += BlockHeader::kSize;
        return size;
    }

    bool VersionMessage::WriteMessage(ByteBuffer* bytes) const{
        bytes->PutLong(timestamp_);
        bytes->PutShort(static_cast<uint16_t>(client_type_));
        bytes->PutString(version_);
        bytes->PutString(nonce_);
        bytes->PutString(node_id_);
        head_.Encode(bytes);
        return true;
    }

    Handle<VerackMessage> VerackMessage::NewInstance(ByteBuffer* bytes){
        //Timestamp timestamp = bytes->GetLong();
        /*std::string node_id = bytes->GetString();
        std::string version = bytes->GetString();
        std::string nonce = bytes->GetString();*/
        Timestamp timestamp = GetCurrentTimestamp();
        std::string node_id = "";
        std::string version = "";
        std::string nonce = "";
        ClientType client_type = ClientType::kClient; //static_cast<ClientType>(bytes->GetUnsignedInt());

        NodeAddress address; //TODO: decode callback_
        BlockHeader head = BlockHeader(bytes);
        return new VerackMessage(client_type, node_id, version, nonce, address, head, timestamp);
    }

    intptr_t VerackMessage::GetMessageSize() const{
        intptr_t size = 0;
        //size += sizeof(Timestamp); // timestamp_
        /*size += sizeof(uint32_t) + node_id_.length(); // node_id_
        size += sizeof(uint32_t) + version_.length(); // version_
        size += sizeof(uint32_t) + nonce_.length(); // nonce_*/
        //size += sizeof(uint32_t); // client_type
        //TODO: calculate sizeof(callback_)
        size += BlockHeader::kSize;
        return size;
    }

    bool VerackMessage::WriteMessage(ByteBuffer* bytes) const{
        //bytes->PutLong(timestamp_);
        /*bytes->PutString(node_id_);
        bytes->PutString(version_);
        bytes->PutString(nonce_);*/
        //bytes->PutUnsignedInt(static_cast<uint32_t>(GetClientType()));
        //TODO: encode(callback_)
        head_.Encode(bytes);
        return true;
    }

    intptr_t PaxosMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += (sizeof(uint32_t) + node_.length()); // node_
        size += proposal_->GetBufferSize();
        return size;
    }

    bool PaxosMessage::WriteMessage(ByteBuffer* bytes) const{
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

    intptr_t TransactionMessage::GetMessageSize() const{
        return 0; //TODO: implement TransactionMessage::GetMessageSize()
    }

    bool TransactionMessage::WriteMessage(ByteBuffer* bytes) const{
        return false; //TODO: implement TransactionMessage::WriteMessage(ByteBuffer*)
    }

    Handle<BlockMessage> BlockMessage::NewInstance(ByteBuffer* bytes){
        Handle<Block> blk = Block::NewInstance(bytes);
        return new BlockMessage(blk);
    }

    intptr_t BlockMessage::GetMessageSize() const{
        return 0; //TODO: implement BlockMessage::GetMessageSize()
    }

    bool BlockMessage::WriteMessage(ByteBuffer* bytes) const{
        return false; //TODO: implement BlockMessage::WriteMessage(ByteBuffer*)
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

    intptr_t InventoryMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(uint32_t); // length(items_)
        size += (items_.size() * InventoryItem::kBufferSize);
        return size;
    }

    bool InventoryMessage::WriteMessage(ByteBuffer* bytes) const{
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

    bool GetBlocksMessage::WriteMessage(ByteBuffer* bytes) const{
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

    intptr_t NotFoundMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += InventoryItem::kBufferSize; // item_
        size += sizeof(uint32_t); // length(message_)
        size += message_.length(); // message_
        return size;
    }

    bool NotFoundMessage::WriteMessage(ByteBuffer* bytes) const{
        bytes->PutShort(static_cast<uint16_t>(item_.GetType()));
        bytes->PutHash(item_.GetHash());
        bytes->PutString(message_);
        return true;
    }

    Handle<GetUnclaimedTransactionsMessage> GetUnclaimedTransactionsMessage::NewInstance(ByteBuffer* bytes){
        std::string user = bytes->GetString();
        return new GetUnclaimedTransactionsMessage(user);
    }

    intptr_t GetUnclaimedTransactionsMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(uint32_t); // length(user_)
        size += user_.length();
        return size;
    }

    bool GetUnclaimedTransactionsMessage::WriteMessage(ByteBuffer* bytes) const{
        bytes->PutString(user_);
        return true;
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

    intptr_t PrepareMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool PrepareMessage::WriteMessage(ByteBuffer* bytes) const{
        return PaxosMessage::WriteMessage(bytes);
    }

    intptr_t PromiseMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool PromiseMessage::WriteMessage(ByteBuffer* bytes) const{
        return PaxosMessage::WriteMessage(bytes);
    }

    intptr_t CommitMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool CommitMessage::WriteMessage(ByteBuffer* bytes) const{
        return PaxosMessage::WriteMessage(bytes);
    }

    intptr_t AcceptedMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool AcceptedMessage::WriteMessage(ByteBuffer* bytes) const{
        return PaxosMessage::WriteMessage(bytes);
    }

    intptr_t RejectedMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool RejectedMessage::WriteMessage(ByteBuffer* bytes) const{
        return PaxosMessage::WriteMessage(bytes);
    }

    intptr_t GetBlocksMessage::GetMessageSize() const{
        return uint256_t::kSize * 2;
    }

    intptr_t GetDataMessage::GetMessageSize() const{
        return uint256_t::kSize;//TODO fixme
    }

    bool GetDataMessage::WriteMessage(ByteBuffer* bytes) const{
        return false; //TODO: fixme
    }
}