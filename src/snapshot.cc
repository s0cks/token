#include "token.h"
#include "bytes.h"
#include "snapshot.h"
#include "snapshot_reader.h"
#include "snapshot_writer.h"

namespace Token{
    bool Snapshot::WriteSnapshot(Snapshot* snapshot){
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
        SnapshotBlockChainSection chain = GetBlockChainSection();
        for(auto& it : chain.index_){
            Handle<Block> blk = GetBlock(it.second);
            if(!vis->Visit(blk)) return false;
        }
        return true;
    }

    IndexReference* Snapshot::GetReference(const Token::uint256_t& hash){
        return blocks_.GetReference(hash);
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

    bool SnapshotPrologueSection::Accept(SnapshotWriter* writer){
        writer->WriteLong(timestamp_);
        writer->WriteString(version_);
        return true;
    }

    bool SnapshotPrologueSection::Accept(SnapshotReader* reader){
        timestamp_ = reader->ReadLong();
        version_ = reader->ReadString();
        return true;
    }

    class BlockChainIndexTableWriter : public BlockChainVisitor{
    private:
        SnapshotWriter* writer_;
        IndexTable& table_;

        inline SnapshotWriter*
        GetWriter() const{
            return writer_;
        }

        inline bool
        HasReference(const uint256_t& hash){
            return table_.find(hash) != table_.end();
        }

        inline IndexReference*
        GetReference(const uint256_t& hash){
            auto pos = table_.find(hash);
            return &pos->second;
        }

        inline IndexReference*
        CreateNewReference(const uint256_t& hash){
            if(HasReference(hash)) return GetReference(hash);
            uint64_t write_pos = GetWriter()->GetCurrentPosition();
            if(!table_.insert({ hash, IndexReference(hash, 0, write_pos, 0) }).second){
                LOG(WARNING) << "couldn't create index reference for " << hash << " @" << write_pos;
                return nullptr;
            }
            return GetReference(hash);
        }
    public:
        BlockChainIndexTableWriter(IndexTable& table, SnapshotWriter* writer):
            BlockChainVisitor(),
            writer_(writer),
            table_(table){}
        ~BlockChainIndexTableWriter() = default;

        bool Visit(const BlockHeader& blk){
            uint256_t hash = blk.GetHash();
            IndexReference* ref = CreateNewReference(hash);
            LOG(INFO) << "created reference for " << hash << ": " << (*ref);
            GetWriter()->WriteReference((*ref));
            return true;
        }
    };

    class BlockChainDataWriter : public BlockChainDataVisitor{
    private:
        SnapshotWriter* writer_;
        IndexTable& table_;

        inline SnapshotWriter*
        GetWriter() const{
            return writer_;
        }

        inline bool
        HasReference(const uint256_t& hash){
            return table_.find(hash) != table_.end();
        }

        inline IndexReference*
        GetReference(const uint256_t& hash){
            auto pos = table_.find(hash);
            return &pos->second;
        }

        inline void
        WriteBlockData(const Handle<Block>& blk){
            size_t size = blk->GetBufferSize();
            ByteBuffer bytes(size);
            if(!blk->Encode(&bytes)){
                LOG(WARNING) << "couldn't serialize block to byte array";
                return;
            }
            GetWriter()->WriteBytes(bytes.data(), size);
        }
    public:
        BlockChainDataWriter(IndexTable& table, SnapshotWriter* writer):
            BlockChainDataVisitor(),
            writer_(writer),
            table_(table){}
        ~BlockChainDataWriter() = default;

        bool Visit(const Handle<Block>& blk){
            uint256_t hash = blk->GetHash();
            if(!HasReference(hash)){
                LOG(WARNING) << "cannot find reference for " << hash << " in index table";
                return false;
            }

            IndexReference* ref = GetReference(hash);
            ref->SetDataPosition(GetWriter()->GetCurrentPosition());
            ref->SetSize(blk->GetBufferSize());
            WriteBlockData(blk);
            return true;
        }
    };

    class BlockChainIndexTableUpdater : public BlockChainVisitor{
    private:
        SnapshotWriter* writer_;
        IndexTable& table_;

        inline SnapshotWriter*
        GetWriter() const{
            return writer_;
        }

        inline bool
        HasReference(const uint256_t& hash){
            return table_.find(hash) != table_.end();
        }

        inline IndexReference*
        GetReference(const uint256_t& hash){
            auto pos = table_.find(hash);
            return &pos->second;
        }
    public:
        BlockChainIndexTableUpdater(IndexTable& table, SnapshotWriter* writer):
            BlockChainVisitor(),
            writer_(writer),
            table_(table){}
        ~BlockChainIndexTableUpdater() = default;

        bool Visit(const BlockHeader& blk){
            uint256_t hash = blk.GetHash();
            if(!HasReference(hash)){
                LOG(WARNING) << "cannot find index table reference to: " << hash;
                return false;
            }

            IndexReference* ref = GetReference(hash);
            int64_t current_pos = GetWriter()->GetCurrentPosition();
            int64_t index_pos = ref->GetIndexPosition();
            GetWriter()->SetCurrentPosition(index_pos);
            GetWriter()->WriteReference((*ref));
            GetWriter()->SetCurrentPosition(current_pos);
            return true;
        }
    };

    bool SnapshotBlockChainSection::WriteBlockIndexTable(SnapshotWriter* writer){
        writer->WriteLong(BlockChain::GetHead().GetHeight() + 1); // num_references
        LOG(INFO) << "writing block chain index table to snapshot...";
        BlockChainIndexTableWriter tbl_writer(index_, writer);
        BlockChain::Accept(&tbl_writer);
        return true;
    }

    bool SnapshotBlockChainSection::WriteBlockData(Token::SnapshotWriter* writer){
        LOG(INFO) << "writing block chain data to snapshot...";
        BlockChainDataWriter data_writer(index_, writer);
        BlockChain::Accept(&data_writer);
        return true;
    }

    bool SnapshotBlockChainSection::UpdateBlockIndexTable(Token::SnapshotWriter* writer){
        LOG(INFO) << "updating block chain index table in snapshot....";
        BlockChainIndexTableUpdater tbl_updater(index_, writer);
        BlockChain::Accept(&tbl_updater);
        return true;
    }

    bool SnapshotBlockChainSection::Accept(SnapshotWriter* writer){
        WriteBlockIndexTable(writer); //TODO: check state
        WriteBlockData(writer); //TODO: check state
        UpdateBlockIndexTable(writer); //TODO: check state
        return true;
    }

    bool SnapshotBlockChainSection::ReadBlockIndexTable(SnapshotReader* reader){
        int64_t num_references = reader->ReadLong();
        for(int64_t idx = 0; idx < num_references; idx++){
            IndexReference ref = reader->ReadReference();
            if(!index_.insert({ ref.GetHash(), ref }).second){
                LOG(WARNING) << "couldn't register new reference: " << ref;
                return false;
            }
        }
        return true;
    }

    bool SnapshotBlockChainSection::Accept(SnapshotReader* reader){
        ReadBlockIndexTable(reader); //TODO: check state
        return true;
    }

    IndexReference* SnapshotBlockChainSection::GetReference(const uint256_t& hash){
        auto pos = index_.find(hash);
        if(pos == index_.end()) return nullptr;
        return &pos->second;
    }
}