#include "pool.h"
#include "message.h"
#include "proposal.h"
#include "discovery.h"
#include "unclaimed_transaction.h"

namespace Token{
    VersionMessage* VersionMessage::NewInstance(Buffer* buff){
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
        size += Hash::GetSize(); // nonce_
        size += UUID::kSize; // node_id_
        size += BlockHeader::GetSize(); // head_
        return size;
    }

    bool VersionMessage::Write(Buffer* buff) const{
        buff->PutLong(timestamp_);
        buff->PutInt(static_cast<int32_t>(client_type_));
        version_.Write(buff);
        buff->PutHash(nonce_);
        node_id_.Write(buff);
        head_.Write(buff);
        return true;
    }

    VerackMessage* VerackMessage::NewInstance(Buffer* buff){
        Timestamp timestamp = buff->GetLong();
        ClientType client_type = (ClientType)buff->GetInt();
        Version version(buff);
        Hash nonce = buff->GetHash();
        UUID node_id(buff);
        NodeAddress callback(buff);
        LOG(INFO) << "read callback: " << callback;
        BlockHeader head = BlockHeader(buff);
        return new VerackMessage(client_type, node_id, head, version, callback, nonce, timestamp);
    }

    bool VerackMessage::Write(Buffer* buff) const{
        buff->PutLong(timestamp_);
        buff->PutInt(static_cast<int32_t>(GetClientType()));
        version_.Write(buff);
        buff->PutHash(nonce_);
        node_id_.Write(buff);
        LOG(INFO) << "writing callback: " << callback_;
        callback_.Write(buff);
        head_.Write(buff);
        return true;
    }

    intptr_t VerackMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(Timestamp); // timestamp_
        size += sizeof(int32_t); // client_type_
        size += Version::kSize; // version_
        size += Hash::GetSize(); // nonce_
        size += UUID::kSize; // node_id_
        size += NodeAddress::kSize; // callback_
        size += BlockHeader::GetSize(); // head_
        return size;
    }

    bool InventoryItem::ItemExists() const{
        switch(type_){
            case kTransaction: return ObjectPool::HasTransaction(hash_);
            case kBlock: return BlockChain::HasBlock(hash_) || ObjectPool::HasBlock(hash_);
            case kUnclaimedTransaction: return ObjectPool::HasUnclaimedTransaction(hash_);
            default: return false;
        }
    }

    InventoryMessage* InventoryMessage::NewInstance(Buffer* buff){
        int64_t num_items = buff->GetLong();
        std::vector<InventoryItem> items;
        for(int64_t idx = 0; idx < num_items; idx++)
            items.push_back(InventoryItem(buff));
        return new InventoryMessage(items);
    }

    intptr_t InventoryMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(int64_t); // length(items_)
        size += (items_.size() * InventoryItem::GetSize());
        return size;
    }

    bool InventoryMessage::Write(Buffer* buff) const{
        buff->PutLong(GetNumberOfItems());
        for(auto& it : items_)
            it.Encode(buff);
        return true;
    }

    bool GetDataMessage::Write(Buffer* buff) const{
        buff->PutLong(GetNumberOfItems());
        for(auto& it : items_)
            it.Encode(buff);
        return true;
    }

    GetDataMessage* GetDataMessage::NewInstance(Buffer* buff){
        int64_t num_items = buff->GetLong();
        std::vector<InventoryItem> items;
        for(int64_t idx = 0; idx < num_items; idx++)
            items.push_back(InventoryItem(buff));
        return new GetDataMessage(items);
    }

    intptr_t GetDataMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(int64_t); // sizeof(items_);
        size += items_.size() * InventoryItem::GetSize(); // items;
        return size;
    }

    const intptr_t GetBlocksMessage::kMaxNumberOfBlocks = 32;

    GetBlocksMessage* GetBlocksMessage::NewInstance(Buffer* buff){
        Hash start = buff->GetHash();
        Hash stop = buff->GetHash();
        return new GetBlocksMessage(start, stop);
    }

    bool GetBlocksMessage::Write(Buffer* buff) const{
        buff->PutHash(start_);
        buff->PutHash(stop_);
        return true;
    }

    NotFoundMessage* NotFoundMessage::NewInstance(Buffer* buff){
        std::string message = ""; //TODO: buff->GetString();
        return new NotFoundMessage(message);
    }

    intptr_t NotFoundMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += InventoryItem::GetSize(); // item_
        size += sizeof(uint32_t); // length(message_)
        size += message_.length(); // message_
        return size;
    }

    bool NotFoundMessage::Write(Buffer* buff) const{
        buff->PutShort(static_cast<uint16_t>(item_.GetType()));
        buff->PutHash(item_.GetHash());
        buff->PutString(message_);
        return true;
    }

    ProposalPtr PaxosMessage::GetProposal() const{
        if(BlockDiscoveryThread::HasProposal()){
            ProposalPtr proposal = BlockDiscoveryThread::GetProposal();
            if(proposal->GetRaw() == GetRaw())
                return proposal;
            return nullptr; //TODO: invalid state?
        }

        ProposalPtr proposal = std::make_shared<Proposal>(raw_);
        BlockDiscoveryThread::SetProposal(proposal);
        return proposal;
    }

    intptr_t PrepareMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool PrepareMessage::Write(Buffer* buff) const{
        return PaxosMessage::Write(buff);
    }

    intptr_t PromiseMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool PromiseMessage::Write(Buffer* buff) const{
        return PaxosMessage::Write(buff);
    }

    intptr_t CommitMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool CommitMessage::Write(Buffer* buff) const{
        return PaxosMessage::Write(buff);
    }

    intptr_t AcceptedMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool AcceptedMessage::Write(Buffer* buff) const{
        return PaxosMessage::Write(buff);
    }

    intptr_t RejectedMessage::GetMessageSize() const{
        return PaxosMessage::GetMessageSize();
    }

    bool RejectedMessage::Write(Buffer* buff) const{
        return PaxosMessage::Write(buff);
    }

    intptr_t GetBlocksMessage::GetMessageSize() const{
        return Hash::GetSize() * 2;
    }

    PeerListMessage* PeerListMessage::NewInstance(Buffer* buff){
        PeerList peers;

        int64_t npeers = buff->GetLong();
        for(int64_t idx = 0; idx < npeers; idx++){
            NodeAddress peer(buff);
            if(!peers.insert(peer).second){
                LOG(WARNING) << "couldn't insert peer: " << peer;
                return nullptr;
            }
        }

        return new PeerListMessage(peers);
    }

    intptr_t PeerListMessage::GetMessageSize() const{
        intptr_t size = 0;
        size += sizeof(int64_t);
        size += (GetNumberOfPeers() * NodeAddress::kSize);
        return size;
    }

    bool PeerListMessage::Write(Buffer* buff) const{
        buff->PutLong(GetNumberOfPeers());
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

    bool GetPeersMessage::Write(Buffer* buff) const{
        //TODO: implement GetPeersMessage::Write(ByteBuffer*)
        return true;
    }
}