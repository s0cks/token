#include "snapshot_block_chain.h"

namespace Token{
    class BlockChainIndexTableWriter : public IndexTableWriter, public BlockChainVisitor{
    public:
        BlockChainIndexTableWriter(SnapshotWriter* writer, IndexTable& table):
                IndexTableWriter(writer, table),
                BlockChainVisitor(){}
        ~BlockChainIndexTableWriter() = default;

        bool Visit(const BlockHeader& blk){
            uint256_t hash = blk.GetHash();
            IndexReference* ref = CreateNewReference(hash);
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
}