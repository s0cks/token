#ifndef TOKEN_SNAPSHOT_H
#define TOKEN_SNAPSHOT_H

#include "common.h"
#include "block_chain.h"

namespace Token{
    class SnapshotBlockIndexVisitor;
    class SnapshotBlockIndex{
        friend class SnapshotBlockLoader; //TODO: revoke access
        friend class SnapshotBlockChainIndexSection;
        friend class SnapshotBlockChainDataSection;
    public:
        class BlockReference{
            friend class SnapshotBlockChainIndexSection; //TODO: revoke access
            friend class SnapshotBlockChainDataSection; //TODO: revoke access
        private:
            uint256_t hash_;
            uint32_t size_;
            uint64_t index_pos_;
            uint64_t data_pos_;

            void SetDataPosition(uint64_t pos){
                data_pos_ = pos;
            }
        public:
            BlockReference(const uint256_t& hash, uint32_t size, uint64_t index_pos, uint64_t data_pos):
                hash_(hash),
                size_(size),
                index_pos_(index_pos),
                data_pos_(data_pos){}
            BlockReference(const BlockHeader& blk, uint32_t size, uint64_t index_pos, uint64_t data_pos):
                BlockReference(blk.GetHash(), size, index_pos, data_pos){}
            BlockReference(const BlockReference& ref):
                hash_(ref.hash_),
                size_(ref.size_),
                index_pos_(ref.index_pos_),
                data_pos_(ref.data_pos_){}

            uint256_t GetHash() const{
                return hash_;
            }

            uint32_t GetSize() const{
                return size_;
            }

            uint64_t GetIndexPosition() const{
                return index_pos_;
            }

            uint64_t GetDataPosition() const{
                return data_pos_;
            }

            void operator=(const BlockReference& ref){
                hash_ = ref.hash_;
                index_pos_ = ref.index_pos_;
                data_pos_ = ref.data_pos_;
                size_ = ref.size_;
            }
        };

        friend std::ostream& operator<<(std::ostream& stream, const BlockReference& ref){
            stream << ref.GetHash() << "@" << ref.GetIndexPosition() << " (" << ref.GetSize() << ") => " << ref.GetDataPosition();
            return stream;
        }
    private:
        std::map<uint256_t, BlockReference> references_;

        BlockReference* GetReference(const uint256_t& hash){
            auto pos = references_.find(hash);
            if(pos == references_.end()){
                LOG(WARNING) << "couldn't find reference " << hash;
                return nullptr;
            }
            return &pos->second;
        }

        BlockReference* CreateReference(const uint256_t& hash, uint32_t size, uint64_t index_pos, uint64_t data_pos=0){
            if(!references_.insert({ hash, BlockReference(hash, size, index_pos, data_pos) }).second){
                LOG(WARNING) << "couldn't create reference for " << hash << " @ " << index_pos;
                return nullptr;
            }
            return GetReference(hash);
        }
    public:
        SnapshotBlockIndex():
            references_(){}
        ~SnapshotBlockIndex(){}

        void Accept(SnapshotBlockIndexVisitor* vis);
    };

    class SnapshotBlockIndexVisitor{
    public:
        SnapshotBlockIndexVisitor() = default;
        virtual ~SnapshotBlockIndexVisitor() = default;
        virtual bool Visit(SnapshotBlockIndex::BlockReference* reference) = 0;
    };

    class SnapshotFile;
    class SnapshotReader;
    class SnapshotWriter;
    class SnapshotBlockDataVisitor;
    class Snapshot{
        friend class SnapshotFile;
        friend class SnapshotPrologueSection; //TODO: revoke access
    private:
        std::string filename_;

        // Prologue Information
        int64_t timestamp_;
        std::string version_;

        // TODO: cleanup index usage for snapshots
        SnapshotBlockIndex* index_;
    public:
        Snapshot():
            filename_(),
            timestamp_(GetCurrentTimestamp()),
            version_(),
            index_(new SnapshotBlockIndex()){}
        Snapshot(SnapshotFile* file);
        ~Snapshot(){
            delete index_;
        }

        std::string GetFilename() const{
            return filename_;
        }

        uint64_t GetTimestamp() const{
            return timestamp_;
        }

        std::string GetVersion() const{
            return version_;
        }

        SnapshotBlockIndex* GetIndex() const{
            return index_;
        }

        Handle<Block> GetBlock(const uint256_t& hash);
        void Accept(SnapshotBlockDataVisitor* vis);

        static bool WriteSnapshot(Snapshot* snapshot);
        static bool WriteNewSnapshot();
        static Snapshot* ReadSnapshot(const std::string& filename);
    };

    class SnapshotBlockDataVisitor{
    protected:
        SnapshotBlockDataVisitor() = default;
    public:
        virtual ~SnapshotBlockDataVisitor() = default;
        virtual bool Visit(const Handle<Block>& blk) = 0;
    };

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

        bool Accept(SnapshotWriter* writer);
        bool Accept(SnapshotReader* reader);
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

        void WriteReference(SnapshotBlockIndex::BlockReference* ref);
        SnapshotBlockIndex::BlockReference* CreateReference(const uint256_t& hash, size_t size);
        SnapshotBlockIndex::BlockReference* ReadReference();
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

        bool Accept(SnapshotWriter* writer);
        bool Accept(SnapshotReader* reader);
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

        void WriteBlockData(const Handle<Block>& blk);

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

        bool Visit(const Handle<Block>& blk);
        bool Accept(SnapshotWriter* writer);
        bool Accept(SnapshotReader* reader);
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

        bool Visit(SnapshotBlockIndex::BlockReference* ref);
        bool Accept(SnapshotWriter* writer);
    };
}

#endif //TOKEN_SNAPSHOT_H
