#ifndef TOKEN_SNAPSHOT_READER_H
#define TOKEN_SNAPSHOT_READER_H

#include "snapshot.h"
#include "crash_report.h"

namespace Token{
    class Snapshot;
    class SnapshotReader{
    private:
        std::string filename_;
        FILE* file_;

        inline FILE*
        GetFilePointer() const{
            return file_;
        }

        int64_t GetCurrentPosition();
    public:
        SnapshotReader(const std::string& filename):
            filename_(filename),
            file_(NULL){
            if((file_ = fopen(filename.c_str(), "rb")) == NULL){
                std::stringstream ss;
                ss << "Couldn't open snapshot file: " << filename << ": " << strerror(errno);
                CrashReport::GenerateAndExit(ss);
            }
        }
        ~SnapshotReader(){
            if(GetFilePointer() != NULL) Close();
        }

        std::string GetFilename() const{
            return filename_;
        }

        Snapshot* ReadSnapshot();
        bool ReadMessage(google::protobuf::Message& msg);
        bool ReadBytes(uint8_t* bytes, size_t size);
        std::string ReadString();
        uint256_t ReadHash();
        int32_t ReadInt();
        uint32_t ReadUnsignedInt();
        int64_t ReadLong();
        uint64_t ReadUnsignedLong();
        IndexReference ReadReference();
        void SetCurrentPosition(int64_t pos);
        void Close();
    };
}

#endif //TOKEN_SNAPSHOT_READER_H