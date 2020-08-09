#ifndef TOKEN_SNAPSHOT_LOADER_H
#define TOKEN_SNAPSHOT_LOADER_H

#include "snapshot_file.h"

namespace Token{
    class SnapshotBlockLoader{
    private:
        SnapshotBlockIndex* index_;
        SnapshotFile file_;

        inline SnapshotBlockIndex*
        GetIndex() const{
            return index_;
        }

        inline std::string
        GetFilename() const{
            return file_.GetFilename();
        }
    public:
        SnapshotBlockLoader(const std::string& filename, SnapshotBlockIndex* index):
            file_(filename),
            index_(index){}
        SnapshotBlockLoader(Snapshot* snapshot):
            SnapshotBlockLoader(snapshot->GetFilename(), snapshot->GetIndex()){}
        ~SnapshotBlockLoader() = default;

        Handle<Block> GetBlock(const uint256_t& hash){
            SnapshotBlockIndex::BlockReference* ref = nullptr;
            if(!(ref = GetIndex()->GetReference(hash))){
                LOG(WARNING) << "cannot find block " << hash << " in snapshot: " << GetFilename();
                return nullptr;
            }

            file_.SetCurrentFilePosition(ref->GetDataPosition());
            size_t size = ref->GetSize();
            uint8_t bytes[size];
            if(!file_.ReadBytes(bytes, size)){
                LOG(WARNING) << "couldn't ready " << size << " block bytes from snapshot: " << GetFilename();
                return nullptr;
            }

            return Block::NewInstance(bytes);
        }
    };
}

#endif //TOKEN_SNAPSHOT_LOADER_H
