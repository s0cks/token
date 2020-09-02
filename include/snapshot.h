#ifndef TOKEN_SNAPSHOT_H
#define TOKEN_SNAPSHOT_H

#include "token.h"
#include "common.h"
#include "block_chain.h"

namespace Token{
    enum SnapshotSection{
        kSnapshotPrologue,
        kSnapshotBlockChainData,
        kSnapshotUnclaimedTransactionPoolData
    };

    typedef uint64_t SnapshotSectionHeader;

    SnapshotSectionHeader CreateSnapshotSectionHeader(SnapshotSection section, uint32_t size);
    SnapshotSection GetSnapshotSection(SnapshotSectionHeader header);
    uint32_t GetSnapshotSectionSize(SnapshotSectionHeader header);

    class SnapshotBlockDataVisitor;
    class SnapshotUnclaimedTransactionDataVisitor;
    class Snapshot : public Object{
        //TODO:
        // - encode bloom filter to blocks
        friend class SnapshotWriter;
        friend class SnapshotReader;
        friend class SnapshotInspector;
        friend class SnapshotBlockChainInitializer;
    private:
        std::string filename_;
        uint64_t timestamp_;
        std::string version_;
        uint256_t head_;

        uint64_t blocks_len_;
        Block** blocks_;

        Snapshot(const std::string& filename):
            filename_(filename),
            timestamp_(),
            version_(),
            head_(),
            blocks_(nullptr){}

        void SetTimestamp(uint64_t timestamp){
            timestamp_ = timestamp;
        }

        void SetVersion(const std::string& version){
            version_ = version;
        }

        void SetHead(const uint256_t& hash){
            head_ = hash;
        }

        void SetNumberOfBlocks(uint64_t num_blocks){
            if(blocks_) free(blocks_);
            blocks_ = (Block**)malloc(sizeof(Block*)*num_blocks);
            memset(blocks_, 0, sizeof(Block*)*num_blocks);
            blocks_len_ = num_blocks;
        }

        void SetBlock(uint64_t idx, const Handle<Block>& blk){
            WriteBarrier(&blocks_[idx], blk);
        }

        static inline void
        CheckSnapshotDirectory(){
            std::string filename = GetSnapshotDirectory();
            if(!FileExists(filename)){
                if(!CreateDirectory(filename)){
                    std::stringstream ss;
                    ss << "Couldn't create snapshots repository: " << filename;
                    CrashReport::GenerateAndExit(ss);
                }
            }
        }
    protected:
        void Accept(WeakReferenceVisitor* vis){
            for(uint64_t idx = 0; idx < blocks_len_; idx++){
                if(!vis->Visit(&blocks_[idx])) return;
            }
        }
    public:
        ~Snapshot();

        std::string GetFilename() const{
            return filename_;
        }

        uint64_t GetTimestamp() const{
            return timestamp_;
        }

        std::string GetVersion() const{
            return version_;
        }

        uint256_t GetHead() const{
            return head_;
        }

        uint64_t GetNumberOfBlocks() const{
            return blocks_len_;
        }

        Handle<Block> GetBlock(uint64_t height) const{
            return blocks_[height];
        }

        std::string ToString() const;
        bool Accept(SnapshotBlockDataVisitor* vis);
        bool Accept(SnapshotUnclaimedTransactionDataVisitor* vis);
        size_t GetBufferSize() const{ return 0; } //TODO: implement Snapshot::GetBufferSize()
        bool Encode(ByteBuffer* bytes) const{ return false; } //TODO: implement Snapshot::Encode(ByteBuffer*)

        static bool WriteNewSnapshot();
        static Snapshot* ReadSnapshot(const std::string& filename);

        static inline std::string
        GetSnapshotDirectory(){
            return (TOKEN_BLOCKCHAIN_HOME + "/snapshots");
        }
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
