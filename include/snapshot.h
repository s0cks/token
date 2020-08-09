#ifndef TOKEN_SNAPSHOT_H
#define TOKEN_SNAPSHOT_H

#include "common.h"
#include "block.h"

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
            uint64_t index_pos_;
            uint64_t data_pos_;

            void SetDataPosition(uint64_t pos){
                data_pos_ = pos;
            }
        public:
            BlockReference(const uint256_t& hash, uint64_t index_pos, uint64_t data_pos):
                hash_(hash),
                index_pos_(index_pos),
                data_pos_(data_pos){}
            BlockReference(const BlockHeader& blk, uint64_t index_pos, uint64_t data_pos):
                BlockReference(blk.GetHash(), index_pos, data_pos){}
            BlockReference(const BlockReference& ref):
                hash_(ref.hash_),
                index_pos_(ref.index_pos_),
                data_pos_(ref.data_pos_){}

            uint256_t GetHash() const{
                return hash_;
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
            }
        };

        friend std::ostream& operator<<(std::ostream& stream, const BlockReference& ref){
            stream << ref.GetHash() << "@" << ref.GetIndexPosition() << " => " << ref.GetDataPosition();
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

        BlockReference* CreateReference(const uint256_t& hash, uint64_t index_pos, uint64_t data_pos=0){
            if(!references_.insert({ hash, BlockReference(hash, index_pos, data_pos) }).second){
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
    class SnapshotBlockDataVisitor;
    class Snapshot{
        friend class SnapshotFile;
        friend class SnapshotPrologueSection; //TODO: revoke access
    private:
        std::string filename_;
        // Prologue Information
        uint32_t timestamp_;
        std::string version_;
        BlockHeader head_;
        SnapshotBlockIndex* index_;
    public:
        Snapshot():
            filename_(),
            timestamp_(0),
            version_(),
            head_(),
            index_(new SnapshotBlockIndex()){}
        Snapshot(SnapshotFile* file);
        ~Snapshot(){
            delete index_;
        }

        std::string GetFilename() const{
            return filename_;
        }

        uint32_t GetTimestamp() const{
            return timestamp_;
        }

        std::string GetVersion() const{
            return version_;
        }

        BlockHeader GetHead() const{
            return head_;
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
}

#endif //TOKEN_SNAPSHOT_H
