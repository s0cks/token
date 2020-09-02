#include "token.h"
#include "bytes.h"
#include "bitfield.h"
#include "snapshot.h"
#include "snapshot_writer.h"
#include "snapshot_reader.h"

namespace Token{
    enum SnapshotSectionHeaderLayout{
        kSectionTypeFieldPosition = 0,
        kSectionTypeFieldSize = 16,
        kSectionSizeFieldPosition = kSectionTypeFieldPosition+kSectionTypeFieldSize,
        kSectionSizeFieldSize = 32
    };

    class SnapshotSectionTypeField : public BitField<SnapshotSectionHeader, SnapshotSection, kSectionTypeFieldPosition, kSectionTypeFieldSize>{};
    class SnapshotSectionSizeField : public BitField<SnapshotSectionHeader, uint32_t, kSectionSizeFieldPosition, kSectionSizeFieldSize>{};

    SnapshotSectionHeader CreateSnapshotSectionHeader(SnapshotSection section, uint32_t size){
        SnapshotSectionHeader header = 0;
        header = SnapshotSectionTypeField::Update(section, header);
        header = SnapshotSectionSizeField::Update(size, header);
        return header;
    }

    SnapshotSection GetSnapshotSection(SnapshotSectionHeader header){
        return (SnapshotSection)SnapshotSectionTypeField::Decode(header);
    }

    uint32_t GetSnapshotSectionSize(SnapshotSectionHeader header){
        return SnapshotSectionSizeField::Decode(header);
    }

    Snapshot::~Snapshot(){}

    std::string Snapshot::ToString() const{
        std::stringstream ss;
        ss << "Snapshot(" << GetFilename() << ")";
        return ss.str();
    }

    bool Snapshot::WriteNewSnapshot(){
        CheckSnapshotDirectory();
        SnapshotWriter writer;
        return writer.WriteSnapshot();
    }

    Snapshot* Snapshot::ReadSnapshot(const std::string& filename){
        SnapshotReader reader(filename);
        return reader.ReadSnapshot();
    }

    bool Snapshot::Accept(SnapshotBlockDataVisitor* vis){
        for(uint64_t idx = 0; idx < GetNumberOfBlocks(); idx++){
            if(!vis->Visit(GetBlock(idx)))
                return false;
        }
        return true;
    }

    bool Snapshot::Accept(SnapshotUnclaimedTransactionDataVisitor* vis){
        //TODO: implement
        return false;
    }
}