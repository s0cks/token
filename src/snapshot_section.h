#ifndef TOKEN_SNAPSHOT_SECTIONS_H
#define TOKEN_SNAPSHOT_SECTIONS_H

#include "token.h"
#include "snapshot.h"
#include "block_chain.h"
#include "snapshot_file.h"

namespace Token{
    class SnapshotSection{
        //TODO:
        // - write section metadata:
        //      * Type
        //      * Size
        // - update prologue section to be more descriptive
    protected:
        Snapshot* snapshot_;

        SnapshotSection(Snapshot* snapshot):
                snapshot_(snapshot){}
    public:
        virtual ~SnapshotSection() = default;

        Snapshot* GetSnapshot() const{
            return snapshot_;
        }

        virtual bool Accept(SnapshotReader* reader) = 0;
        virtual bool Accept(SnapshotWriter* writer) = 0;
    };

    class SnapshotPrologueSection : public SnapshotSection{
    public:
        SnapshotPrologueSection(): SnapshotSection(nullptr){}
        SnapshotPrologueSection(Snapshot* snapshot): SnapshotSection(snapshot){}
        ~SnapshotPrologueSection() = default;

        bool Accept(SnapshotWriter* writer){
            SnapshotFile* file = writer->GetFile();
            file->WriteInt(GetCurrentTime());
            file->WriteString(Token::GetVersion());
            return true;
        }

        bool Accept(SnapshotReader* reader){
            Snapshot* snapshot = GetSnapshot();
            SnapshotFile* file = reader->GetFile();

            snapshot->timestamp_ = file->ReadInt();
            snapshot->version_ = file->ReadString();
            return true;
        }
    };

    class SnapshotBlockChainIndexSection : public SnapshotSection, public BlockChainDataVisitor{
    private:
        SnapshotFile* file_;
        SnapshotBlockIndex* index_;

        inline void
        SetFile(SnapshotFile* file){
            file_ = file;
        }

        inline SnapshotFile*
        GetFile() const{
            return file_;
        }

        inline SnapshotBlockIndex*
        GetIndex() const{
            return index_;
        }

        inline void
        WriteReference(SnapshotBlockIndex::BlockReference* ref){
            LOG(INFO) << "writing reference: " << (*ref);
            GetFile()->WriteHash(ref->GetHash());
            GetFile()->WriteLong(ref->GetDataPosition());
            GetFile()->WriteInt(ref->GetSize());
        }

        inline SnapshotBlockIndex::BlockReference*
        CreateReference(const uint256_t& hash, size_t size){ // need to find a more intelligent way of mapping sizes
            return GetIndex()->CreateReference(hash, size,GetFile()->GetCurrentFilePosition());
        }

        inline SnapshotBlockIndex::BlockReference*
        ReadReference(){
            uint64_t index_pos = GetFile()->GetCurrentFilePosition();
            uint256_t hash = GetFile()->ReadHash();
            uint64_t data_pos = GetFile()->ReadLong();
            uint32_t size = GetFile()->ReadInt();
            return GetIndex()->CreateReference(hash, size, index_pos, data_pos);
        }
    public:
        SnapshotBlockChainIndexSection(SnapshotBlockIndex* index):
            SnapshotSection(nullptr),
            file_(nullptr),
            index_(index){}
        SnapshotBlockChainIndexSection(Snapshot* snapshot):
            SnapshotSection(snapshot),
            file_(nullptr),
            index_(snapshot->GetIndex()){}
        ~SnapshotBlockChainIndexSection() = default;

        bool Visit(const Handle<Block>& blk){
            uint256_t hash = blk->GetHash();
            SnapshotBlockIndex::BlockReference* ref = CreateReference(hash, blk->GetBufferSize());
            WriteReference(ref);
            return true;
        }

        bool Accept(SnapshotWriter* writer){
            SnapshotFile* file = writer->GetFile();
            SetFile(file);

            uint32_t num_references = BlockChain::GetHead().GetHeight() + 1; // does this really capture the num references?
            file->WriteInt(num_references);

            LOG(INFO) << "writing " << num_references << " references to snapshot block index";
            BlockChain::Accept(this);
            return true;
        }

        bool Accept(SnapshotReader* reader){
            SnapshotFile* file = reader->GetFile();
            SetFile(file);

            uint32_t num_references = file->ReadInt();
            for(uint32_t idx = 0; idx < num_references; idx++) ReadReference();
            return true;
        }
    };

    class SnapshotBlockChainDataSection : public SnapshotSection, BlockChainDataVisitor{
        //TODO:
        // - need to iterate block chain genesis->head
    private:
        SnapshotFile* file_;
        SnapshotBlockIndex* index_;

        inline void
        SetFile(SnapshotFile* file){
            file_ = file;
        }

        inline SnapshotFile*
        GetFile() const{
            return file_;
        }

        inline SnapshotBlockIndex*
        GetIndex() const{
            return index_;
        }

        inline void
        WriteBlockData(const Handle<Block>& blk){
            size_t size = blk->GetBufferSize();
            uint8_t bytes[size];
            if(!blk->Encode(bytes)){
                LOG(WARNING) << "couldn't serialize block to byte array";
                return;
            }
            GetFile()->WriteBytes(bytes, size);
        }

        inline SnapshotBlockIndex::BlockReference*
        GetReference(const uint256_t& hash){
            return GetIndex()->GetReference(hash);
        }
    public:
        SnapshotBlockChainDataSection(SnapshotBlockIndex* index):
            SnapshotSection(nullptr),
            file_(nullptr),
            index_(index){}
        SnapshotBlockChainDataSection(Snapshot* snapshot):
            SnapshotSection(snapshot),
            file_(nullptr),
            index_(snapshot->GetIndex()){}
        ~SnapshotBlockChainDataSection() = default;

        bool Visit(const Handle<Block>& blk){
            SnapshotFile* file = GetFile();
            SnapshotBlockIndex* index = GetIndex();

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

        bool Accept(SnapshotWriter* writer){
            SetFile(writer->GetFile());
            BlockChain::Accept(this);
            return true;
        }

        bool Accept(SnapshotReader* reader){
            return true; //TODO implement SnapshotBlockDataSection read()
        }
    };

    class SnapshotBlockIndexLinker : public SnapshotBlockIndexVisitor{
    private:
        SnapshotFile* file_;
        SnapshotBlockIndex* index_;

        inline void
        SetFile(SnapshotFile* file){
            file_ = file;
        }

        inline SnapshotFile*
        GetFile() const{
            return file_;
        }

        inline SnapshotBlockIndex*
        GetIndex() const{
            return index_;
        }
    public:
        SnapshotBlockIndexLinker(SnapshotBlockIndex* index):
            SnapshotBlockIndexVisitor(),
            file_(nullptr),
            index_(index){}
        ~SnapshotBlockIndexLinker() = default;

        bool Visit(SnapshotBlockIndex::BlockReference* ref){
            SnapshotFile* file = GetFile();

            uint64_t current_pos = file->GetCurrentFilePosition();
            uint64_t index_pos = ref->GetIndexPosition();
            uint64_t data_pos = ref->GetDataPosition();

            file->SetCurrentFilePosition(index_pos + uint256_t::kSize);
            file->WriteLong(data_pos);
            file->SetCurrentFilePosition(current_pos);
            return true;
        }

        bool Accept(SnapshotWriter* writer){
            SetFile(writer->GetFile());
            GetIndex()->Accept(this);
            return true;
        }
    };
}

#endif //TOKEN_SNAPSHOT_SECTIONS_H
