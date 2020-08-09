#include "token.h"
#include "snapshot.h"
#include "snapshot_file.h"
#include "snapshot_loader.h"

#include "snapshot_reader.h"
#include "snapshot_writer.h"

namespace Token{
    void SnapshotBlockIndex::Accept(SnapshotBlockIndexVisitor* vis){
        for(auto& ref : references_){
            if(!vis->Visit(&ref.second)) return;
        }
    }

    Snapshot::Snapshot(SnapshotFile* file):
        filename_(file->GetFilename()),
        timestamp_(0),
        version_(),
        index_(new SnapshotBlockIndex()){}

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

    class SnapshotBlockIndexReferenceLoader : public SnapshotBlockIndexVisitor{
    private:
        SnapshotBlockDataVisitor* parent_;
        SnapshotBlockLoader loader_;

        inline SnapshotBlockDataVisitor*
        GetParent() const{
            return parent_;
        }

        inline Handle<Block>
        LoadBlock(const uint256_t& hash){
            return loader_.GetBlock(hash);
        }
    public:
        SnapshotBlockIndexReferenceLoader(Snapshot* snapshot, SnapshotBlockDataVisitor* parent):
            SnapshotBlockIndexVisitor(),
            parent_(parent),
            loader_(snapshot){}
        ~SnapshotBlockIndexReferenceLoader() = default;

        bool Visit(SnapshotBlockIndex::BlockReference* ref){
            uint256_t hash = ref->GetHash();
            Handle<Block> blk = LoadBlock(hash);
            return GetParent()->Visit(blk);
        }
    };

    Handle<Block> Snapshot::GetBlock(const uint256_t& hash){
        std::string filename = GetFilename();
        if(!FileExists(filename)){
            LOG(WARNING) << "snapshot " << filename << " doesn't exist, cannot load block data";
            return nullptr;
        }
        SnapshotBlockLoader loader(this);
        return loader.GetBlock(hash);
    }

    void Snapshot::Accept(SnapshotBlockDataVisitor* vis){
        std::string filename = GetFilename();
        if(!FileExists(filename)){
            LOG(WARNING) << "snapshot " << filename << " doesn't exist, cannot load block data";
            return;
        }
        SnapshotBlockIndexReferenceLoader loader(this, vis);
        GetIndex()->Accept(&loader);
    }

    bool SnapshotPrologueSection::Accept(SnapshotWriter* writer){
        SnapshotFile* file = writer->GetFile();
        file->WriteLong(GetCurrentTimestamp());
        file->WriteString(Token::GetVersion());
        return true;
    }

    bool SnapshotPrologueSection::Accept(SnapshotReader* reader){
        Snapshot* snapshot = GetSnapshot();
        SnapshotFile* file = reader->GetFile();

        snapshot->timestamp_ = file->ReadLong();
        snapshot->version_ = file->ReadString();
        return true;
    }

    void SnapshotBlockChainIndexSection::WriteReference(SnapshotBlockIndex::BlockReference* ref){
        GetFile()->WriteHash(ref->GetHash());
        GetFile()->WriteUnsignedLong(ref->GetDataPosition());
        GetFile()->WriteInt(ref->GetSize());
    }

    SnapshotBlockIndex::BlockReference* SnapshotBlockChainIndexSection::CreateReference(const uint256_t& hash, size_t size){ // need to find a more intelligent way of mapping sizes
        return GetIndex()->CreateReference(hash, size,GetFile()->GetCurrentFilePosition());
    }

    SnapshotBlockIndex::BlockReference* SnapshotBlockChainIndexSection::ReadReference(){
        uint64_t index_pos = GetFile()->GetCurrentFilePosition();
        uint256_t hash = GetFile()->ReadHash();
        uint64_t data_pos = GetFile()->ReadUnsignedLong();
        uint32_t size = GetFile()->ReadInt();
        return GetIndex()->CreateReference(hash, size, index_pos, data_pos);
    }

    bool SnapshotBlockChainIndexSection::Accept(SnapshotWriter* writer){
        SnapshotFile* file = writer->GetFile();
        SetFile(file);

        uint32_t num_references = BlockChain::GetHead().GetHeight() + 1; // does this really capture the num references?
        file->WriteInt(num_references);
        BlockChain::Accept(this);
        return true;
    }

    bool SnapshotBlockChainIndexSection::Accept(SnapshotReader* reader){
        SnapshotFile* file = reader->GetFile();
        SetFile(file);

        uint32_t num_references = file->ReadInt();
        for(uint32_t idx = 0; idx < num_references; idx++) ReadReference();
        return true;
    }

    void SnapshotBlockChainDataSection::WriteBlockData(const Handle<Block>& blk){
        size_t size = blk->GetBufferSize();
        uint8_t bytes[size];
        if(!blk->Encode(bytes)){
            LOG(WARNING) << "couldn't serialize block to byte array";
            return;
        }
        GetFile()->WriteBytes(bytes, size);
    }

    bool SnapshotBlockChainDataSection::Visit(const Handle<Block>& blk){
        SnapshotFile* file = GetFile();

        uint256_t hash = blk->GetHash();
        SnapshotBlockIndex::BlockReference* ref = nullptr;
        if(!(ref = GetReference(hash))){
            LOG(WARNING) << "couldn't find reference " << hash << " in snapshot block index";
            return false;
        }

        ref->SetDataPosition(file->GetCurrentFilePosition());
        LOG(INFO) << "writing block of size " << ref->GetSize() << " @" << ref->GetDataPosition();
        WriteBlockData(blk);
        return true;
    }

    bool SnapshotBlockChainDataSection::Accept(SnapshotWriter* writer){
        SetFile(writer->GetFile());
        BlockChain::Accept(this);
        return true;
    }

    bool SnapshotBlockChainDataSection::Accept(SnapshotReader* reader){
        return true; //TODO implement SnapshotBlockDataSection read()
    }

    bool SnapshotBlockIndexLinker::Visit(SnapshotBlockIndex::BlockReference* ref){
        SnapshotFile* file = GetFile();

        uint64_t current_pos = file->GetCurrentFilePosition();
        uint64_t index_pos = ref->GetIndexPosition();
        uint64_t data_pos = ref->GetDataPosition();

        file->SetCurrentFilePosition(index_pos + uint256_t::kSize);
        file->WriteUnsignedLong(data_pos);
        file->SetCurrentFilePosition(current_pos);
        return true;
    }

    bool SnapshotBlockIndexLinker::Accept(SnapshotWriter* writer){
        SetFile(writer->GetFile());
        GetIndex()->Accept(this);
        return true;
    }
}