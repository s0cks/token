#ifndef TOKEN_SNAPSHOT_H
#define TOKEN_SNAPSHOT_H

#include "token.h"
#include "common.h"
#include "block_chain.h"

namespace Token{
#define SNAPSHOT_PROLOGUE 0xAA
#define SNAPSHOT_BLOCKDATA 0xAB
#define SNAPSHOT_UTXODATA 0xAC

    class SnapshotWriter;
    class SnapshotReader;
    class SnapshotSection{
        friend class Snapshot;
        friend class SnapshotWriter;
        friend class SnapshotReader;
    public:
        enum Section{
            kPrologueSection,
            kBlockChainDataSection,
            kUnclaimedTransactionPoolSection,
        };
    protected:
        Section section_;
        int64_t offset_;
        int64_t size_;

        SnapshotSection(Section sid):
            section_(sid),
            offset_(0),
            size_(0){}

        void SetRegion(int64_t offset, int64_t size){
            offset_ = offset;
            size_ = size;
        }
    public:
        virtual ~SnapshotSection() = default;

        Section GetSectionId() const{
            return section_;
        }

        int64_t GetOffset() const{
            return offset_;
        }

        int64_t GetSize() const{
            return size_;
        }

        virtual bool Accept(SnapshotWriter* writer) = 0;
        virtual bool Accept(SnapshotReader* reader) = 0;
    };

    class SnapshotPrologueSection : public SnapshotSection{
    private:
        std::string version_;
        uint64_t timestamp_;
    public:
        SnapshotPrologueSection():
            SnapshotSection(SnapshotSection::kPrologueSection),
            version_(Token::GetVersion()),
            timestamp_(GetCurrentTimestamp()){}
        ~SnapshotPrologueSection() = default;

        std::string GetVersion() const{
            return version_;
        }

        uint64_t GetTimestamp() const{
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

    class IndexedSection : public SnapshotSection{
    protected:
        IndexTable index_;

        IndexedSection(SnapshotSection::Section section):
            SnapshotSection(section),
            index_(){}

        virtual bool LinkIndexTable(SnapshotWriter* writer) = 0;
        virtual bool WriteIndexTable(SnapshotWriter* writer) = 0;
        virtual bool WriteData(SnapshotWriter* writer) = 0;
        bool ReadIndexTable(SnapshotReader* reader);
    public:
        virtual ~IndexedSection() = default;

        IndexReference* GetReference(const uint256_t& hash);

        size_t GetNumberOfReferences() const{
            return index_.size();
        }

        bool Accept(SnapshotWriter* writer){
            WriteIndexTable(writer);
            WriteData(writer);
            LinkIndexTable(writer);
            return true;
        }

        bool Accept(SnapshotReader* reader){
            ReadIndexTable(reader); //TODO: check state
            return true;
        }

        void operator=(const IndexedSection& section){
            index_ = section.index_;
        }
    };

    class SnapshotBlockChainSection : public IndexedSection{
        friend class Snapshot;
    protected:
        bool WriteIndexTable(SnapshotWriter* writer);
        bool WriteData(SnapshotWriter* writer);
        bool LinkIndexTable(SnapshotWriter* writer);
    public:
        SnapshotBlockChainSection(): IndexedSection(SnapshotSection::kBlockChainDataSection){}
        ~SnapshotBlockChainSection() = default;

        void operator=(const SnapshotBlockChainSection& section){
            IndexedSection::operator=(section);
        }
    };

    class SnapshotUnclaimedTransactionPoolSection : public IndexedSection{
        friend class Snapshot;
    protected:
        bool WriteIndexTable(SnapshotWriter* writer);
        bool WriteData(SnapshotWriter* writer);
        bool LinkIndexTable(SnapshotWriter* writer);
    public:
        SnapshotUnclaimedTransactionPoolSection():
            IndexedSection(SnapshotSection::kUnclaimedTransactionPoolSection){}
        ~SnapshotUnclaimedTransactionPoolSection() = default;

        void operator=(const SnapshotUnclaimedTransactionPoolSection& section){
            IndexedSection::operator=(section);
        }
    };

    class SnapshotBlockDataVisitor;
    class SnapshotUnclaimedTransactionDataVisitor;
    class Snapshot{
        //TODO:
        // - encode bloom filter to blocks
        friend class SnapshotWriter;
        friend class SnapshotReader;
        friend class SnapshotInspector;
    private:
        std::string filename_;
        SnapshotPrologueSection prologue_;
        SnapshotBlockChainSection blocks_;
        SnapshotUnclaimedTransactionPoolSection utxos_;

        IndexReference* GetReference(const uint256_t& hash);
        Handle<Block> GetBlock(const IndexReference& ref);
        Handle<UnclaimedTransaction> GetUnclaimedTransaction(const IndexReference& ref);

        Handle<Block> GetBlock(const uint256_t& hash){
            IndexReference* ref = GetReference(hash);
            if(!ref) return nullptr;
            return GetBlock((*ref));
        }

        Handle<UnclaimedTransaction> GetUnclaimedTransaction(const uint256_t& hash){
            IndexReference* ref = GetReference(hash);
            if(!ref) return nullptr;
            return GetUnclaimedTransaction((*ref));
        }
    public:
        Snapshot():
            filename_(),
            prologue_(),
            blocks_(),
            utxos_(){}
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

        SnapshotUnclaimedTransactionPoolSection GetUnclaimedTransactionPoolSection() const{
            return utxos_;
        }

        bool Accept(SnapshotBlockDataVisitor* vis);
        bool Accept(SnapshotUnclaimedTransactionDataVisitor* vis);

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

    class SnapshotUnclaimedTransactionDataVisitor{
    protected:
        SnapshotUnclaimedTransactionDataVisitor() = default;
    public:
        virtual ~SnapshotUnclaimedTransactionDataVisitor() = default;
        virtual bool Visit(const Handle<UnclaimedTransaction>& utxo) = 0;
    };
}

#endif //TOKEN_SNAPSHOT_H
