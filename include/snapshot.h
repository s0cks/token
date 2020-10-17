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

    class SnapshotVisitor;
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
        Hash head_;

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

        void SetHead(const Hash& hash){
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
                if(!CreateDirectory(filename))
                    LOG(WARNING) << "couldn't create snapshots directory: " << filename;
            }
        }
    protected:
        bool Accept(WeakObjectPointerVisitor* vis){
            for(uint64_t idx = 0; idx < blocks_len_; idx++){
                if(!vis->Visit(&blocks_[idx]))
                    return false;
            }
            return true;
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

        Hash GetHead() const{
            return head_;
        }

        uint64_t GetNumberOfBlocks() const{
            return blocks_len_;
        }

        Handle<Block> GetBlock(uint64_t height) const{
            return blocks_[height];
        }

        Handle<Block> GetBlock(const Hash& hash) const{
            for(uint64_t idx = 0; idx < blocks_len_; idx++){
                if(blocks_[idx]->GetHash() == hash) return blocks_[idx];
            }
            return nullptr;
        }

        std::string ToString() const;
        bool Accept(SnapshotVisitor* vis);
        size_t GetBufferSize() const{ return 0; } //TODO: implement Snapshot::GetBufferSize()
        bool Encode(ByteBuffer* bytes) const{ return false; } //TODO: implement Snapshot::Encode(ByteBuffer*)

        static bool WriteNewSnapshot();
        static Snapshot* ReadSnapshot(const std::string& filename);

        static inline std::string
        GetSnapshotDirectory(){
            return (TOKEN_BLOCKCHAIN_HOME + "/snapshots");
        }
    };

    class SnapshotVisitor{
    protected:
        SnapshotVisitor() = default;
    public:
        virtual ~SnapshotVisitor() = default;
        virtual bool VisitStart(Snapshot* snapshot){ return true; }
        virtual bool Visit(const Handle<Block>& blk) = 0;
        virtual bool VisitEnd(Snapshot* snapshot){ return true; }
    };

    class SnapshotPrinter : public SnapshotVisitor{
    private:
        SnapshotPrinter():
            SnapshotVisitor(){}
    public:
        ~SnapshotPrinter() = default;

        bool VisitStart(Snapshot* snapshot){
            LOG(INFO) << "Snapshot: " << snapshot->GetFilename();
            LOG(INFO) << "Generated: " << GetTimestampFormattedReadable(snapshot->GetTimestamp());
            LOG(INFO) << "Blocks:";
            return true;
        }

        bool Visit(const Handle<Block>& blk){
            LOG(INFO) << " - " << blk;
            return true;
        }

        static bool Print(Snapshot* snapshot){
            SnapshotPrinter printer;
            return snapshot->Accept(&printer);
        }
    };
}

#endif //TOKEN_SNAPSHOT_H
