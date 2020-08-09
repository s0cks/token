#ifndef TOKEN_SNAPSHOT_H
#define TOKEN_SNAPSHOT_H

#include "common.h"
#include "block.h"

namespace Token{
    class Snapshot{
        //TODO: revoke access
        friend class SnapshotPrologueSection;
    private:
        // Prologue Information
        uint32_t timestamp_;
        std::string version_;
        BlockHeader head_;
    public:
        Snapshot():
            timestamp_(0),
            version_(),
            head_(){}
        ~Snapshot(){}

        uint32_t GetTimestamp() const{
            return timestamp_;
        }

        std::string GetVersion() const{
            return version_;
        }

        BlockHeader GetHead() const{
            return head_;
        }

        static bool WriteSnapshot(Snapshot* snapshot);
        static bool WriteNewSnapshot();
        static Snapshot* ReadSnapshot(const std::string& filename);
    };
}

#endif //TOKEN_SNAPSHOT_H
