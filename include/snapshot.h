#ifndef TOKEN_SNAPSHOT_H
#define TOKEN_SNAPSHOT_H

#include <google/protobuf/message.h>
#include "common.h"
#include "block.h"

namespace Token{
    class Snapshot{
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

    class SnapshotFile{
        friend class SnapshotWriter;
        friend class SnapshotReader;
    private:
        std::string filename_;
        FILE* file_;

        SnapshotFile(const std::string& filename):
            filename_(filename),
            file_(NULL){
            LOG(INFO) << "opening snapshot file: " << filename;
            if((file_ = fopen(filename_.c_str(), "rb")) == NULL){
                std::stringstream ss;
                ss << "Couldn't open snapshot file " << filename_ << strerror(errno);
                CrashReport::GenerateAndExit(ss);
            }
        }
        SnapshotFile():
            filename_(GetNewSnapshotFilename()),
            file_(NULL){
            LOG(INFO) << "creating snapshot file: " << filename_;

            CheckSnapshotDirectory();
            if((file_ = fopen(filename_.data(), "wb")) == NULL){
                std::stringstream ss;
                ss << "Couldn't create snapshot file " << filename_ << ": " << strerror(errno);
                CrashReport::GenerateAndExit(ss);
            }
        }

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

        std::string GetFilename() const{
            return filename_;
        }

        bool ReadMessage(google::protobuf::Message& msg);
        bool ReadBytes(uint8_t* bytes, size_t size);
        uint8_t ReadByte();
        uint32_t ReadInt();
        std::string ReadString();

        void WriteMessage(google::protobuf::Message& msg);
        void WriteBytes(uint8_t* bytes, size_t size);
        void WriteInt(uint32_t value);
        void WriteString(const std::string& value);

        void Flush();
        void Close();
    };

    class SnapshotWriter{
        friend class SnapshotSection;
    private:
        SnapshotFile file_;
    public:
        SnapshotWriter():
            file_(){}
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

#endif //TOKEN_SNAPSHOT_H
