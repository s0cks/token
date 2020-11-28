#include "message.h"
#include "proposal.h"

#include "block_pool.h"
#include "transaction_pool.h"
#include "unclaimed_transaction_pool.h"

namespace Token{
    Handle<VersionMessage> VersionMessage::NewInstance(const Handle<Buffer>& buff){
        Timestamp timestamp = buff->GetLong();
        ClientType client_type = static_cast<ClientType>(buff->GetInt());
        Version version(buff);
        Hash nonce = buff->GetHash();
        UUID node_id(buff);
        BlockHeader head = BlockHeader(buff);
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

    bool VersionMessage::Write(const Handle<Buffer>& buff) const{
        buff->PutLong(timestamp_);
        buff->PutInt(static_cast<int32_t>(client_type_));
        version_.Write(buff);
        buff->PutHash(nonce_);
        node_id_.Write(buff);
        head_.Write(buff);
        return true;
    }

    Handle<VerackMessage> VerackMessage::NewInstance(const Handle<Buffer>& buff){
        Timestamp timestamp = buff->GetLong();
        ClientType client_type = (ClientType)buff->GetInt();
        Version version(buff);
        Hash nonce = buff->GetHash();
        UUID node_id(buff);
        NodeAddress callback(buff);
        BlockHeader head = BlockHeader(buff);
        return new VerackMessage(client_type, node_id, version, nonce, callback, head, timestamp);
    }

    bool VerackMessage::Write(const Handle<Buffer>& buff) const{
        buff->PutLong(timestamp_);
        buff->PutInt(static_cast<int32_t>(GetClientType()));
        version_.Write(buff);
        buff->PutHash(nonce_);
        node_id_.Write(buff);
        callback_.Write(buff);
        head_.Write(buff);
        return true;
    }

    intptr_t VerackMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(Timestamp); // timestamp_
        size += sizeof(int32_t); // client_type_
        size += Version::kSize; // version_
        size += Hash::kSize; // nonce_
        size += UUID::kSize; // node_id_
        size += NodeAddress::kSize; // callback_
        size += BlockHeader::kSize; // head_
        return size;
    }

    intptr_t PaxosMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += UUID::kSize;
        size += proposal_->GetBufferSize();
        return size;
    }

    bool PaxosMessage::Write(const Handle<Buffer>& buff) const{
        proposer_.Write(buff);
        proposal_->Encode(buff);
        return true;
    }

    Handle<PrepareMessage> PrepareMessage::NewInstance(const Handle<Buffer>& buff){
        UUID proposer(buff);
        Handle<Proposal> proposal = Proposal::NewInstance(buff);
        return new PrepareMessage(proposer, proposal);
    }

    Handle<PromiseMessage> PromiseMessage::NewInstance(const Handle<Buffer>& buff){
        UUID proposer(buff);
        Handle<Proposal> proposal = Proposal::NewInstance(buff);
        return new PromiseMessage(proposer, proposal);
    }

    Handle<CommitMessage> CommitMessage::NewInstance(const Handle<Buffer>& buff){
        UUID proposer(buff);
        Handle<Proposal> proposal = Proposal::NewInstance(buff);
        return new CommitMessage(proposer, proposal);
    }

    Handle<AcceptedMessage> AcceptedMessage::NewInstance(const Handle<Buffer>& buff){
        UUID proposer(buff);
        Handle<Proposal> proposal = Proposal::NewInstance(buff);
        return new AcceptedMessage(proposer, proposal);
    }

    Handle<RejectedMessage> RejectedMessage::NewInstance(const Handle<Buffer>& buff){
        UUID proposer(buff);
        Handle<Proposal> proposal = Proposal::NewInstance(buff);
        return new RejectedMessage(proposer, proposal);
    }

    Handle<TransactionMessage> TransactionMessage::NewInstance(const Handle<Buffer>& buff){
        Handle<Transaction> tx = Transaction::NewInstance(buff);
        return new TransactionMessage(tx);
    }

    intptr_t TransactionMessage::GetMessageSize() const{
        return 0; //TODO: implement TransactionMessage::GetMessageSize()
    }

    bool TransactionMessage::Write(const Handle<Buffer>& buff) const{
        return false; //TODO: implement TransactionMessage::Write(ByteBuffer*)
    }

    Handle<BlockMessage> BlockMessage::NewInstance(const Handle<Buffer>& buff){
        Handle<Block> blk = Block::NewInstance(buff);
        return new BlockMessage(blk);
    }

    intptr_t BlockMessage::GetMessageSize() const{
        return data_->GetBufferSize();
    }

    bool BlockMessage::Write(const Handle<Buffer>& buff) const{
        data_->Write(buff);
        return true;
    }

    std::string BlockMessage::ToString() const{
        std::stringstream ss;
        ss << "BlockMessage(" << GetBlock() << ")";
        return ss.str();
    }

    Handle<UnclaimedTransactionMessage> UnclaimedTransactionMessage::NewInstance(const Handle<Buffer>& buff){
        Handle<UnclaimedTransaction> utxo = UnclaimedTransaction::NewInstance(buff);
        return new UnclaimedTransactionMessage(utxo);
    }

    intptr_t UnclaimedTransactionMessage::GetMessageSize() const{
        return data_->GetBufferSize();
    }

    bool UnclaimedTransactionMessage::Write(const Handle<Buffer>& buff) const{
        data_->Write(buff);
        return true;
    }

    bool InventoryItem::ItemExists() const{
        switch(type_){
            case kTransaction: return TransactionPool::HasTransaction(hash_);
            case kBlock: return BlockChain::HasBlock(hash_) || BlockPool::HasBlock(hash_);
            case kUnclaimedTransaction: return UnclaimedTransactionPool::HasUnclaimedTransaction(hash_);
            default: return false;
        }
    }

    Handle<InventoryMessage> InventoryMessage::NewInstance(const Handle<Buffer>& buff){
        uint32_t num_items = buff->GetInt();
        std::vector<InventoryItem> items;
        DecodeItems(buff, items, num_items);
        return new InventoryMessage(items);
    }

    void InventoryMessage::DecodeItems(const Handle<Buffer>& buff, std::vector<InventoryItem>& items, uint32_t num_items){
        for(uint32_t idx = 0; idx < num_items; idx++){
            InventoryItem::Type type = static_cast<InventoryItem::Type>(buff->GetShort());
            Hash hash = buff->GetHash();
            items.push_back(InventoryItem(type, hash));
        }
    }

    intptr_t InventoryMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(uint32_t); // length(items_)
        size += (items_.size() * InventoryItem::kSize);
        return size;
    }

    bool InventoryMessage::Write(const Handle<Buffer>& buff) const{
        buff->PutInt(items_.size());
        for(auto& it : items_){
            buff->PutShort(static_cast<uint16_t>(it.GetType()));
            buff->PutHash(it.GetHash());
        }
        return true;
    }

    bool GetDataMessage::Write(const Handle<Buffer>& buff) const{
        buff->PutLong(items_.size());
        for(auto& it : items_){
            buff->PutInt(static_cast<int32_t>(it.GetType()));
            buff->PutHash(it.GetHash());
        }
        return true;
    }

    intptr_t GetDataMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(int64_t); // sizeof(items_);
        size += items_.size() * InventoryItem::kSize; // items;
        return size;
    }

    Handle<GetDataMessage> GetDataMessage::NewInstance(const Handle<Buffer>& buff){
        std::vector<InventoryItem> items;
        int64_t num_items = buff->GetLong();
        LOG(INFO) << "reading " << num_items << " items";
        for(int64_t n = 0; n < num_items; n++){
            int32_t type = buff->GetInt();
            Hash hash = buff->GetHash();

            LOG(INFO) << "type: " << type;
            LOG(INFO) << "hash: " << hash;
            InventoryItem item(static_cast<InventoryItem::Type>(type), hash);
            LOG(INFO) << "parsed item: " << item;
            items.push_back(item);
        }
        return new GetDataMessage(items);
    }

    const intptr_t GetBlocksMessage::kMaxNumberOfBlocks = 32;

    Handle<GetBlocksMessage> GetBlocksMessage::NewInstance(const Handle<Buffer>& buff){
        Hash start = buff->GetHash();
        Hash stop = buff->GetHash();
        return new GetBlocksMessage(start, stop);
    }

    bool GetBlocksMessage::Write(const Handle<Buffer>& buff) const{
        buff->PutHash(start_);
        buff->PutHash(stop_);
        return true;
    }

    Handle<NotFoundMessage> NotFoundMessage::NewInstance(const Handle<Buffer>& buff){
        std::string message = ""; //TODO: buff->GetString();
        return new NotFoundMessage(message);
    }

    intptr_t NotFoundMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += InventoryItem::kSize; // item_
        size += sizeof(uint32_t); // length(message_)
        size += message_.length(); // message_
        return size;
    }

    bool NotFoundMessage::Write(const Handle<Buffer>& buff) const{
        buff->PutShort(static_cast<uint16_t>(item_.GetType()));
        buff->PutHash(item_.GetHash());
        buff->PutString(message_);
        return true;
    }

    Handle<GetUnclaimedTransactionsMessage> GetUnclaimedTransactionsMessage::NewInstance(const Handle<Buffer>& buff){
        User user = buff->GetUser();
        return new GetUnclaimedTransactionsMessage(user);
    }

    intptr_t GetUnclaimedTransactionsMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += User::kSize;
        return size;
    }

    bool GetUnclaimedTransactionsMessage::Write(const Handle<Buffer>& buff) const{
        buff->PutUser(user_);
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

    bool PrepareMessage::Write(const Handle<Buffer>& buff) const{
        return PaxosMessage::Write(buff);
    }

    intptr_t PromiseMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool PromiseMessage::Write(const Handle<Buffer>& buff) const{
        return PaxosMessage::Write(buff);
    }

    intptr_t CommitMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool CommitMessage::Write(const Handle<Buffer>& buff) const{
        return PaxosMessage::Write(buff);
    }

    intptr_t AcceptedMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool AcceptedMessage::Write(const Handle<Buffer>& buff) const{
        return PaxosMessage::Write(buff);
    }

    intptr_t RejectedMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool RejectedMessage::Write(const Handle<Buffer>& buff) const{
        return PaxosMessage::Write(buff);
    }

    intptr_t GetBlocksMessage::GetMessageSize() const{
        return Hash::kSize * 2;
    }

    Handle<PeerListMessage> PeerListMessage::NewInstance(const Handle<Buffer>& buff){
        PeerList peers;

        int32_t npeers = buff->GetInt();
        for(int32_t idx = 0; idx < npeers; idx++){
            Peer peer(buff);
            if(!peers.insert(peer).second){
                LOG(WARNING) << "couldn't insert peer: " << peer;
                return nullptr;
            }
        }

        return new PeerListMessage(peers);
    }

    intptr_t PeerListMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(int32_t);
        size += (GetNumberOfPeers() * Peer::kSize);
        return size;
    }

    bool PeerListMessage::Write(const Handle<Buffer>& buff) const{
        buff->PutInt(GetNumberOfPeers());
        for(auto it = peers_begin();
            it != peers_end();
            it++){
            it->Write(buff);
        }
        return true;
    }

    intptr_t GetPeersMessage::GetMessageSize() const{
        //TODO: implement GetPeersMessage::GetMessageSize()
        return 0;
    }

    bool GetPeersMessage::Write(const Handle<Buffer>& buff) const{
        //TODO: implement GetPeersMessage::Write(ByteBuffer*)
        return true;
    }
}