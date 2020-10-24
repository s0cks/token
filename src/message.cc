#include "message.h"
#include "proposal.h"

#include "byte_buffer.h"

namespace Token{
    Handle<VersionMessage> VersionMessage::NewInstance(ByteBuffer* bytes){
        Timestamp timestamp = bytes->GetLong();
        ClientType client_type = static_cast<ClientType>(bytes->GetInt());
        Version version(bytes);
        Hash nonce = bytes->GetHash();
        UUID node_id(bytes);
        BlockHeader head = BlockHeader(bytes);
        return new VersionMessage(client_type, version, node_id, timestamp, nonce, head);
    }

    intptr_t VersionMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(Timestamp); // timestamp_
        size += sizeof(int32_t); // client_type_
        size += Version::kSize; // version_
        size += Hash::kSize; // nonce_
        size += UUID::kSize; // node_id_
        size += BlockHeader::kSize; // head_
        return size;
    }

    bool VersionMessage::WriteMessage(ByteBuffer* bytes) const{
        bytes->PutLong(timestamp_);
        bytes->PutInt(static_cast<int32_t>(client_type_));
        version_.Write(bytes);
        bytes->PutHash(nonce_);
        node_id_.Write(bytes);
        head_.Write(bytes);
        return true;
    }

    Handle<VerackMessage> VerackMessage::NewInstance(ByteBuffer* bytes){
        Timestamp timestamp = bytes->GetLong();
        ClientType client_type = static_cast<ClientType>(bytes->GetInt());
        Version version(bytes);
        Hash nonce = bytes->GetHash();
        UUID node_id(bytes);
        NodeAddress address; //TODO: decode callback_
        BlockHeader head = BlockHeader(bytes);
        return new VerackMessage(client_type, node_id, version, nonce, address, head, timestamp);
    }

    intptr_t VerackMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(Timestamp); // timestamp_
        size += sizeof(int32_t); // client_type_
        size += Version::kSize; // version_
        size += Hash::kSize; // nonce_
        size += UUID::kSize; // node_id_
        //TODO: calculate sizeof(callback_)
        size += BlockHeader::kSize; // head_
        return size;
    }

    bool VerackMessage::WriteMessage(ByteBuffer* bytes) const{
        bytes->PutLong(timestamp_);
        bytes->PutInt(static_cast<int32_t>(GetClientType()));
        version_.Write(bytes);
        bytes->PutHash(nonce_);
        node_id_.Write(bytes);
        head_.Write(bytes);
        return true;
    }

    intptr_t PaxosMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += UUID::kSize;
        size += proposal_->GetBufferSize();
        return size;
    }

    bool PaxosMessage::WriteMessage(ByteBuffer* bytes) const{
        proposer_.Write(bytes);
        proposal_->Encode(bytes);
        return true;
    }

    Handle<PrepareMessage> PrepareMessage::NewInstance(ByteBuffer* bytes){
        UUID proposer(bytes);
        Handle<Proposal> proposal = Proposal::NewInstance(bytes);
        return new PrepareMessage(proposer, proposal);
    }

    Handle<PromiseMessage> PromiseMessage::NewInstance(ByteBuffer* bytes){
        UUID proposer(bytes);
        Handle<Proposal> proposal = Proposal::NewInstance(bytes);
        return new PromiseMessage(proposer, proposal);
    }

    Handle<CommitMessage> CommitMessage::NewInstance(ByteBuffer* bytes){
        UUID proposer(bytes);
        Handle<Proposal> proposal = Proposal::NewInstance(bytes);
        return new CommitMessage(proposer, proposal);
    }

    Handle<AcceptedMessage> AcceptedMessage::NewInstance(ByteBuffer* bytes){
        UUID proposer(bytes);
        Handle<Proposal> proposal = Proposal::NewInstance(bytes);
        return new AcceptedMessage(proposer, proposal);
    }

    Handle<RejectedMessage> RejectedMessage::NewInstance(ByteBuffer* bytes){
        UUID proposer(bytes);
        Handle<Proposal> proposal = Proposal::NewInstance(bytes);
        return new RejectedMessage(proposer, proposal);
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
        return data_->GetMessageSize();
    }

    bool BlockMessage::WriteMessage(ByteBuffer* bytes) const{
        data_->Write(bytes);
        return true;
    }

    Handle<UnclaimedTransactionMessage> UnclaimedTransactionMessage::NewInstance(ByteBuffer* bytes){
        Handle<UnclaimedTransaction> utxo = UnclaimedTransaction::NewInstance(bytes);
        return new UnclaimedTransactionMessage(utxo);
    }

    intptr_t UnclaimedTransactionMessage::GetMessageSize() const{
        return data_->GetMessageSize();
    }

    bool UnclaimedTransactionMessage::WriteMessage(ByteBuffer* bytes) const{
        data_->Write(bytes);
        return true;
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
            Hash hash = bytes->GetHash();
            items.push_back(InventoryItem(type, hash));
        }
    }

    intptr_t InventoryMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(uint32_t); // length(items_)
        size += (items_.size() * InventoryItem::kSize);
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

    bool GetDataMessage::WriteMessage(ByteBuffer* bytes) const{
        bytes->PutLong(items_.size());
        for(auto& it : items_){
            bytes->PutInt(static_cast<int32_t>(it.GetType()));
            bytes->PutHash(it.GetHash());
        }
        return true;
    }

    intptr_t GetDataMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(int64_t); // sizeof(items_);
        size += items_.size() * InventoryItem::kSize; // items;
        return size;
    }

    Handle<GetDataMessage> GetDataMessage::NewInstance(ByteBuffer* bytes){
        std::vector<InventoryItem> items;
        int64_t num_items = bytes->GetLong();
        LOG(INFO) << "reading " << num_items << " items";
        for(int64_t n = 0; n < num_items; n++){
            int32_t type = bytes->GetInt();
            Hash hash = bytes->GetHash();

            LOG(INFO) << "type: " << type;
            LOG(INFO) << "hash: " << hash;
            InventoryItem item(static_cast<InventoryItem::Type>(type), hash);
            LOG(INFO) << "parsed item: " << item;
            items.push_back(item);
        }
        return new GetDataMessage(items);
    }

    const intptr_t GetBlocksMessage::kMaxNumberOfBlocks = 32;

    Handle<GetBlocksMessage> GetBlocksMessage::NewInstance(ByteBuffer* bytes){
        Hash start = bytes->GetHash();
        Hash stop = bytes->GetHash();
        return new GetBlocksMessage(start, stop);
    }

    bool GetBlocksMessage::WriteMessage(ByteBuffer* bytes) const{
        bytes->PutHash(start_);
        bytes->PutHash(stop_);
        return true;
    }

    Handle<NotFoundMessage> NotFoundMessage::NewInstance(ByteBuffer* bytes){
        std::string message = bytes->GetString();
        return new NotFoundMessage(message);
    }

    intptr_t NotFoundMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += InventoryItem::kSize; // item_
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
        User user(bytes);
        return new GetUnclaimedTransactionsMessage(user);
    }

    intptr_t GetUnclaimedTransactionsMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += User::kSize;
        return size;
    }

    bool GetUnclaimedTransactionsMessage::WriteMessage(ByteBuffer* bytes) const{
        user_.Encode(bytes);
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
        return Hash::kSize * 2;
    }
}