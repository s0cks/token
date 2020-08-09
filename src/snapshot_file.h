#ifndef TOKEN_SNAPSHOT_FILE_H
#define TOKEN_SNAPSHOT_FILE_H

#include <stdio.h>
#include <string>
#include <glog/logging.h>
#include "snapshot.h"
#include "crash_report.h"

namespace Token{
    class SnapshotFile{
        friend class Snapshot;
        friend class SnapshotWriter;
        friend class SnapshotReader;
        friend class SnapshotBlockLoader;
    private:
        Snapshot* snapshot_;
        std::string filename_;
        FILE* file_;

        SnapshotFile(const std::string& filename);
        SnapshotFile(Snapshot* snapshot);

        inline FILE*
        GetFilePointer() const{
            return file_;
        }

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
            filename << "/snapshot-" << GetCurrentTimeFormatted() << ".dat";
            return filename.str();
        }
    public:
        ~SnapshotFile();

        Snapshot* GetSnapshot() const{
            return snapshot_;
        }

        std::string GetFilename() const{
            return filename_;
        }

        uint64_t GetCurrentFilePosition();
        void SetCurrentFilePosition(uint64_t pos);
        void Flush();
        void Close();
        void WriteHash(const uint256_t& hash);
        void WriteString(const std::string& value);
        void WriteMessage(google::protobuf::Message& msg);
        void WriteBytes(uint8_t* bytes, size_t size);
        void WriteInt(uint32_t value);
        void WriteLong(uint64_t value);
        bool ReadMessage(google::protobuf::Message& msg);
        bool ReadBytes(uint8_t* bytes, size_t size);
        uint8_t ReadByte();
        uint32_t ReadInt();
        uint64_t ReadLong();
        uint256_t ReadHash();
        std::string ReadString();
    };

    class SnapshotWriter{
        friend class SnapshotSection;
    private:
        SnapshotFile file_;

        void WriteBlockChainData();
    public:
        SnapshotWriter(Snapshot* snapshot=new Snapshot()):
            file_(snapshot){}
        ~SnapshotWriter() = default;

        SnapshotFile* GetFile(){
            return &file_;
        }

        bool WriteSnapshot();
    };

    class SnapshotReader{
    private:
        SnapshotFile file_;
    public:
        SnapshotReader(const std::string& filename):
            file_(filename){}
        ~SnapshotReader() = default;

        SnapshotFile* GetFile(){
            return &file_;
        }

        Snapshot* ReadSnapshot();
    };
}

#endif //TOKEN_SNAPSHOT_FILE_H
