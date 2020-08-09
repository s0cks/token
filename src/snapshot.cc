#include "snapshot.h"
#include "snapshot_file.h"
#include "snapshot_loader.h"

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
        head_(),
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
}