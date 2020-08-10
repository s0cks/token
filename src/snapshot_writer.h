#ifndef TOKEN_SNAPSHOT_WRITER_H
#define TOKEN_SNAPSHOT_WRITER_H

#include "snapshot.h"
#include "crash_report.h"

namespace Token{
    class SnapshotWriter{
        friend class SnapshotSection;
    private:
        std::string filename_;
        FILE* file_;
        Snapshot* snapshot_; // TODO: remove snapshot_ instance

        static inline std::string
        GetSnapshotDirectory(){
            return (TOKEN_BLOCKCHAIN_HOME + "/snapshots");
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

        static inline std::string
        GetNewSnapshotFilename(){
            std::stringstream filename;
            filename << GetSnapshotDirectory();
            filename << "/snapshot-" << GetTimestampFormattedFileSafe(GetCurrentTimestamp()) << ".dat";
            return filename.str();
        }

        inline FILE*
        GetFilePointer() const{
            return file_;
        }
    public:
        SnapshotWriter(Snapshot* snapshot, const std::string& filename=GetNewSnapshotFilename()):
            filename_(filename),
            file_(NULL),
            snapshot_(snapshot){
            CheckSnapshotDirectory();
            if((file_ = fopen(filename.c_str(), "wb")) == NULL){
                std::stringstream ss;
                ss << "Couldn't create snapshot file: " << filename << ": " << strerror(errno);
                CrashReport::GenerateAndExit(ss);
            }
        }
        ~SnapshotWriter(){
            if(GetFilePointer() != NULL) Close();
        }

        Snapshot* GetSnapshot() const{
            return snapshot_;
        }

        std::string GetFilename() const{
            return filename_;
        }

        int64_t GetCurrentPosition();
        bool WriteSnapshot();
        void WriteBytes(uint8_t* bytes, size_t size);
        void WriteInt(int32_t value);
        void WriteUnsignedInt(uint32_t value);
        void WriteLong(int64_t value);
        void WriteUnsignedLong(uint64_t value);
        void WriteHash(const uint256_t& hash);
        void WriteString(const std::string& value);
        void WriteReference(const IndexReference& ref);
        void SetCurrentPosition(int64_t pos);
        void Flush();
        void Close();
    };
}

#endif //TOKEN_SNAPSHOT_WRITER_H