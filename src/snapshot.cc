#include "token.h"
#include "bytes.h"
#include "snapshot.h"
#include "snapshot_prologue.h"
#include "snapshot_block_chain.h"
#include "snapshot_unclaimed_transaction_pool.h"

namespace Token{
    void IndexTableDataWriter::WriteData(Token::ByteBuffer* bytes, size_t size){
        GetWriter()->WriteBytes(bytes->data(), size);
    }

    IndexReference* IndexTableWriter::CreateNewReference(const Token::uint256_t& hash){
        if(HasReference(hash)) return GetReference(hash);
        uint64_t write_pos = GetWriter()->GetCurrentPosition();
        if(!table_.insert({ hash, IndexReference(hash, 0, write_pos, 0) }).second){
            LOG(WARNING) << "couldn't create index reference for " << hash << " @" << write_pos;
            return nullptr;
        }
        return GetReference(hash);
    }

    Snapshot::Snapshot():
        filename_(),
        prologue_(new SnapshotPrologueSection()),
        block_chain_(new SnapshotBlockChainSection()),
        utxos_(new SnapshotUnclaimedTransactionPoolSection()){}

    Snapshot::~Snapshot(){
        if(prologue_) delete prologue_;
        if(block_chain_) delete block_chain_;
        if(utxos_) delete utxos_;
    }

    bool Snapshot::WriteSnapshot(Snapshot* snapshot){
        CheckSnapshotDirectory();
        SnapshotWriter writer(snapshot);
        return writer.WriteSnapshot();
    }

    bool Snapshot::WriteNewSnapshot(){
        Snapshot snapshot;
        return WriteSnapshot(&snapshot);
    }

    Snapshot* Snapshot::ReadSnapshot(const std::string& filename){
        SnapshotReader reader(filename);
        return reader.ReadSnapshot();
    }

    bool Snapshot::Accept(SnapshotBlockDataVisitor* vis){
        SnapshotBlockChainSection* chain = GetBlockChainSection();
        for(auto& it : chain->index_){
            Handle<Block> blk = GetBlock(it.second);
            if(!vis->Visit(blk)) return false;
        }
        return true;
    }

    bool Snapshot::Accept(SnapshotUnclaimedTransactionDataVisitor* vis){
        SnapshotUnclaimedTransactionPoolSection* utxos = GetUnclaimedTransactionPoolSection();
        for(auto& it : utxos->index_){
            Handle<UnclaimedTransaction> utxo = GetUnclaimedTransaction(it.second);
            if(!vis->Visit(utxo)) return false;
        }
        return true;
    }

    IndexReference* Snapshot::GetReference(const Token::uint256_t& hash){
        return GetBlockChainSection()->GetReference(hash);
    }

    Handle<Block> Snapshot::GetBlock(const IndexReference& ref){
        SnapshotReader reader(GetFilename());
        reader.SetCurrentPosition(ref.GetDataPosition());
        size_t size = ref.GetSize();
        ByteBuffer bytes(size);
        if(!reader.ReadBytes(bytes.data(), size)){
            LOG(WARNING) << "couldn't read block data " << ref << " from snapshot file: " << GetFilename();
            return nullptr;
        }
        return Block::NewInstance(&bytes);
    }

    Handle<UnclaimedTransaction> Snapshot::GetUnclaimedTransaction(const IndexReference& ref){
        SnapshotReader reader(GetFilename());
        reader.SetCurrentPosition(ref.GetDataPosition());
        size_t size = ref.GetSize();
        ByteBuffer bytes(size);
        if(!reader.ReadBytes(bytes.data(), size)){
            LOG(WARNING) << "couldn't read block data " << ref << " from snapshot file: " << GetFilename();
            return nullptr;
        }
        return UnclaimedTransaction::NewInstance(&bytes);
    }

    IndexReference* IndexedSection::GetReference(const uint256_t& hash){
        auto pos = index_.find(hash);
        if(pos == index_.end()) return nullptr;
        return &pos->second;
    }

    bool IndexedSection::ReadIndexTable(Token::SnapshotReader* reader){
        uint64_t num_references = reader->ReadLong();
        for(uint64_t idx = 0; idx < num_references; idx++){
            IndexReference ref = reader->ReadReference();
            if(!index_.insert({ ref.GetHash(), ref }).second){
                LOG(WARNING) << "couldn't register new reference: " << ref;
                return false;
            }
        }
        return true;
    }
}