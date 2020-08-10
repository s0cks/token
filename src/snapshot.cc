#include "token.h"
#include "bytes.h"
#include "snapshot.h"
#include "snapshot_reader.h"
#include "snapshot_writer.h"

#include "unclaimed_transaction_pool.h"

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

    bool Snapshot::Accept(SnapshotUnclaimedTransactionDataVisitor* vis){
        SnapshotUnclaimedTransactionPoolSection utxos = GetUnclaimedTransactionPoolSection();
        for(auto& it : utxos.index_){
            Handle<UnclaimedTransaction> utxo = GetUnclaimedTransaction(it.second);
            if(!vis->Visit(utxo)) return false;
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

    class IndexTableWriter{
    protected:
        SnapshotWriter* writer_;
        IndexTable& table_;

        IndexTableWriter(SnapshotWriter* writer, IndexTable& table):
            writer_(writer),
            table_(table){}

        SnapshotWriter* GetWriter() const{
            return writer_;
        }

        bool HasReference(const uint256_t& hash){
            return table_.find(hash) != table_.end();
        }

        IndexReference* GetReference(const uint256_t& hash){
            auto pos = table_.find(hash);
            return &pos->second;
        }

        IndexReference* CreateNewReference(const uint256_t& hash){
            if(HasReference(hash)) return GetReference(hash);
            uint64_t write_pos = GetWriter()->GetCurrentPosition();
            if(!table_.insert({ hash, IndexReference(hash, 0, write_pos, 0) }).second){
                LOG(WARNING) << "couldn't create index reference for " << hash << " @" << write_pos;
                return nullptr;
            }
            return GetReference(hash);
        }
    public:
        virtual ~IndexTableWriter() = default;
    };

    class IndexTableDataWriter{
    protected:
        SnapshotWriter* writer_;
        IndexTable& table_;

        IndexTableDataWriter(SnapshotWriter* writer, IndexTable& table):
                writer_(writer),
                table_(table){}

        SnapshotWriter* GetWriter() const{
            return writer_;
        }

        bool HasReference(const uint256_t& hash){
            return table_.find(hash) != table_.end();
        }

        IndexReference* GetReference(const uint256_t& hash){
            auto pos = table_.find(hash);
            return &pos->second;
        }

        void WriteData(ByteBuffer* bytes, size_t size){
            GetWriter()->WriteBytes(bytes->data(), size);
        }
    public:
        virtual ~IndexTableDataWriter() = default;
    };

    class IndexTableLinker{
    protected:
        SnapshotWriter* writer_;
        IndexTable& table_;

        IndexTableLinker(SnapshotWriter* writer, IndexTable& table):
                writer_(writer),
                table_(table){}

        SnapshotWriter* GetWriter() const{
            return writer_;
        }

        bool HasReference(const uint256_t& hash){
            return table_.find(hash) != table_.end();
        }

        IndexReference* GetReference(const uint256_t& hash){
            auto pos = table_.find(hash);
            return &pos->second;
        }
    public:
        virtual ~IndexTableLinker() = default;
    };

    IndexReference* IndexedSection::GetReference(const uint256_t& hash){
        auto pos = index_.find(hash);
        if(pos == index_.end()) return nullptr;
        return &pos->second;
    }

    bool IndexedSection::ReadIndexTable(Token::SnapshotReader* reader){
        int64_t num_references = reader->ReadLong();
        LOG(INFO) << "reading " << num_references << " references from unclaimed transaction pool section";
        for(int64_t idx = 0; idx < num_references; idx++){
            IndexReference ref = reader->ReadReference();
            if(!index_.insert({ ref.GetHash(), ref }).second){
                LOG(WARNING) << "couldn't register new reference: " << ref;
                return false;
            }
        }
        return true;
    }

    class BlockChainIndexTableWriter : public IndexTableWriter, public BlockChainVisitor{
    public:
        BlockChainIndexTableWriter(SnapshotWriter* writer, IndexTable& table):
            IndexTableWriter(writer, table),
            BlockChainVisitor(){}
        ~BlockChainIndexTableWriter() = default;

        bool Visit(const BlockHeader& blk){
            uint256_t hash = blk.GetHash();
            IndexReference* ref = CreateNewReference(hash);
            LOG(INFO) << "created reference for " << hash << ": " << (*ref);
            GetWriter()->WriteReference((*ref));
            return true;
        }
    };

    class BlockChainIndexTableDataWriter : public IndexTableDataWriter, public BlockChainDataVisitor{
    private:
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
        BlockChainIndexTableDataWriter(SnapshotWriter* writer, IndexTable& table):
            IndexTableDataWriter(writer, table),
            BlockChainDataVisitor(){}
        ~BlockChainIndexTableDataWriter() = default;

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

    class BlockChainIndexTableLinker : public IndexTableLinker, public BlockChainVisitor{
    public:
        BlockChainIndexTableLinker(SnapshotWriter* writer, IndexTable& table):
            IndexTableLinker(writer, table),
            BlockChainVisitor(){}
        ~BlockChainIndexTableLinker() = default;

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

    bool SnapshotBlockChainSection::WriteIndexTable(SnapshotWriter* writer){
        writer->WriteLong(BlockChain::GetHead().GetHeight() + 1); // num_references
        LOG(INFO) << "writing block chain index table to snapshot...";
        BlockChainIndexTableWriter tbl_writer(writer, index_);
        BlockChain::Accept(&tbl_writer);
        return true;
    }

    bool SnapshotBlockChainSection::WriteData(Token::SnapshotWriter* writer){
        LOG(INFO) << "writing block chain data to snapshot...";
        BlockChainIndexTableDataWriter data_writer(writer, index_);
        BlockChain::Accept(&data_writer);
        return true;
    }

    bool SnapshotBlockChainSection::LinkIndexTable(Token::SnapshotWriter* writer){
        LOG(INFO) << "updating block chain index table in snapshot....";
        BlockChainIndexTableLinker tbl_link(writer, index_);
        BlockChain::Accept(&tbl_link);
        return true;
    }

    class UnclaimedTransactionPoolIndexTableWriter : public IndexTableWriter, public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolIndexTableWriter(SnapshotWriter* writer, IndexTable& table):
            IndexTableWriter(writer, table),
            UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolIndexTableWriter() = default;

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            uint256_t hash = utxo->GetHash();
            IndexReference* ref = CreateNewReference(hash);
            LOG(INFO) << "created reference for " << hash << ": " << (*ref);
            GetWriter()->WriteReference((*ref));
            return true;
        }
    };

    class UnclaimedTransactionPoolIndexDataTableWriter : public IndexTableDataWriter, public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolIndexDataTableWriter(SnapshotWriter* writer, IndexTable& table):
            IndexTableDataWriter(writer, table),
            UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolIndexDataTableWriter() = default;

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            uint256_t hash = utxo->GetHash();
            if(!HasReference(hash)){
                LOG(WARNING) << "cannot find reference for " << hash << " in index table";
                return false;
            }

            size_t size = utxo->GetBufferSize();
            IndexReference* ref = GetReference(hash);
            ref->SetDataPosition(GetWriter()->GetCurrentPosition());
            ref->SetSize(size);

            ByteBuffer bytes(size);
            if(!utxo->Encode(&bytes)){
                LOG(WARNING) << "cannot serialize unclaimed transaction " << hash << " to snapshot";
                return false;
            }

            WriteData(&bytes, size);
            return true;
        }
    };

    class UnclaimedTransactionPoolIndexTableLinker : public IndexTableLinker, public UnclaimedTransactionPoolVisitor{
    public:
        UnclaimedTransactionPoolIndexTableLinker(SnapshotWriter* writer, IndexTable& table):
            IndexTableLinker(writer, table),
            UnclaimedTransactionPoolVisitor(){}
        ~UnclaimedTransactionPoolIndexTableLinker() = default;

        bool Visit(const Handle<UnclaimedTransaction>& utxo){
            uint256_t hash = utxo->GetHash();
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

    bool SnapshotUnclaimedTransactionPoolSection::WriteIndexTable(SnapshotWriter* writer){
        writer->WriteLong(UnclaimedTransactionPool::GetNumberOfUnclaimedTransactions());
        LOG(INFO) << "writing unclaimed transaction pool index table to snapshot...";
        UnclaimedTransactionPoolIndexTableWriter tbl_writer(writer, index_);
        UnclaimedTransactionPool::Accept(&tbl_writer);
        return true;
    }

    bool SnapshotUnclaimedTransactionPoolSection::WriteData(SnapshotWriter* writer){
        LOG(INFO) << "writing unclaimed transaction pool data to snapshot...";
        UnclaimedTransactionPoolIndexDataTableWriter data_writer(writer, index_);
        UnclaimedTransactionPool::Accept(&data_writer);
        return true;
    }

    bool SnapshotUnclaimedTransactionPoolSection::LinkIndexTable(SnapshotWriter* writer){
        LOG(INFO) << "linking unclaimed transaction pool index table in snapshot....";
        UnclaimedTransactionPoolIndexTableLinker tbl_link(writer, index_);
        UnclaimedTransactionPool::Accept(&tbl_link);
        return true;
    }
}