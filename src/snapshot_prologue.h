#ifndef TOKEN_SNAPSHOT_PROLOGUE_H
#define TOKEN_SNAPSHOT_PROLOGUE_H

#include "snapshot.h"
#include "snapshot_writer.h"
#include "snapshot_reader.h"

namespace Token{
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

        bool Accept(SnapshotWriter* writer){
            writer->WriteLong(timestamp_);
            writer->WriteString(version_);
            return true;
        }

        bool Accept(SnapshotReader* reader){
            timestamp_ = reader->ReadLong();
            version_ = reader->ReadString();
            return true;
        }

        void operator=(const SnapshotPrologueSection& prologue){
            version_ = prologue.version_;
            timestamp_ = prologue.timestamp_;
        }
    };
}

#endif //TOKEN_SNAPSHOT_PROLOGUE_H