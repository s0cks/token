#ifndef TOKEN_SNAPSHOT_H
#define TOKEN_SNAPSHOT_H

#include "token.h"
#include "common.h"
#include "block_chain.h"

namespace Token{
    class SnapshotWriter;
    class SnapshotReader;
    class SnapshotSection{
    protected:
        SnapshotSection() = default;
    public:
        virtual ~SnapshotSection() = default;
        virtual bool Accept(SnapshotWriter* writer) = 0;
        virtual bool Accept(SnapshotReader* reader) = 0;
    };

    class SnapshotPrologueSection : public SnapshotSection{
    private:
        std::string version_;
        int64_t timestamp_;
    public:
        SnapshotPrologueSection():
            SnapshotSection(),
            version_(Token::GetVersion()),
            timestamp_(GetCurrentTimestamp()){}
        ~SnapshotPrologueSection() = default;

        std::string GetVersion() const{
            return version_;
        }

        int64_t GetTimestamp() const{
            return timestamp_;
        }

        bool Accept(SnapshotWriter* writer);
        bool Accept(SnapshotReader* reader);

        void operator=(const SnapshotPrologueSection& prologue){
            version_ = prologue.version_;
            timestamp_ = prologue.timestamp_;
        }
    };

    class IndexReference{
    private:
        uint256_t hash_;
        size_t size_;
        int64_t data_pos_;
        int64_t index_pos_;
    public:
        IndexReference(const uint256_t& hash, size_t size, int64_t index_pos, int64_t data_pos):
            hash_(hash),
            size_(size),
            index_pos_(index_pos),
            data_pos_(data_pos){}
        IndexReference(const BlockHeader& blk, size_t size, int64_t index_pos, int64_t data_pos): IndexReference(blk.GetHash(), size, index_pos, data_pos){}
        IndexReference(const IndexReference& ref):
            hash_(ref.hash_),
            size_(ref.size_),
            index_pos_(ref.index_pos_),
            data_pos_(ref.data_pos_){}
        ~IndexReference() = default;

        uint256_t GetHash() const{
            return hash_;
        }

        size_t GetSize() const{
            return size_;
        }

        void SetSize(size_t size){
            size_ = size;
        }

        int64_t GetIndexPosition() const{
            return index_pos_;
        }

        void SetIndexPosition(int64_t pos){
            index_pos_ = pos;
        }

        int64_t GetDataPosition() const{
            return data_pos_;
        }

        void SetDataPosition(int64_t pos){
            data_pos_ = pos;
        }

        void operator=(const IndexReference& ref){
            hash_ = ref.hash_;
            index_pos_ = ref.index_pos_;
            data_pos_ = ref.data_pos_;
            size_ = ref.size_;
        }

        friend std::ostream& operator<<(std::ostream& stream, const IndexReference& ref){
            stream << ref.GetHash() << "@" << ref.GetIndexPosition() << " (" << ref.GetSize() << ") => " << ref.GetDataPosition();
            return stream;
        }
    };

    typedef std::map<uint256_t, IndexReference> IndexTable;

    class SnapshotBlockChainSection : public SnapshotSection{
        friend class Snapshot;
    private:
        IndexTable index_;

        bool WriteBlockIndexTable(SnapshotWriter* writer);
        bool WriteBlockData(SnapshotWriter* writer);
        bool UpdateBlockIndexTable(SnapshotWriter* writer);
        bool ReadBlockIndexTable(SnapshotReader* reader);
    public:
        SnapshotBlockChainSection():
            SnapshotSection(){}
        ~SnapshotBlockChainSection() = default;

        IndexReference* GetReference(const uint256_t& hash);
        bool Accept(SnapshotWriter* writer);
        bool Accept(SnapshotReader* reader);

        void operator=(const SnapshotBlockChainSection& chain){
            index_ = chain.index_;
        }
    };

    class SnapshotBlockDataVisitor;
    class Snapshot{
        friend class SnapshotWriter;
        friend class SnapshotReader;
        friend class SnapshotInspector;
    private:
        std::string filename_;
        SnapshotPrologueSection prologue_;
        SnapshotBlockChainSection blocks_;

        IndexReference* GetReference(const uint256_t& hash);
        Handle<Block> GetBlock(const IndexReference& ref);

        Handle<Block> GetBlock(const uint256_t& hash){
            IndexReference* ref = GetReference(hash);
            if(!ref) return nullptr;
            return GetBlock((*ref));
        }
    public:
        Snapshot():
            filename_(),
            prologue_(),
            blocks_(){}
        ~Snapshot() = default;

        std::string GetFilename() const{
            return filename_;
        }

        SnapshotPrologueSection GetPrologueSection() const{
            return prologue_;
        }

        SnapshotBlockChainSection GetBlockChainSection() const{
            return blocks_;
        }

        bool Accept(SnapshotBlockDataVisitor* vis);

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
}

#endif //TOKEN_SNAPSHOT_H
