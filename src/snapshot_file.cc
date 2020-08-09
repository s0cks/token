#include "snapshot_file.h"
#include "snapshot_section.h"

namespace Token{
#define CHECK_FILE_POINTER \
    if(GetFilePointer() == NULL){ \
        LOG(WARNING) << "snapshot file " << GetFilename() << " is not opened"; \
        return; \
    }

    SnapshotFile::SnapshotFile(const std::string& filename):
        filename_(filename),
        file_(NULL){
        LOG(INFO) << "opening snapshot file: " << filename;
        if((file_ = fopen(filename_.c_str(), "rb")) == NULL){
            std::stringstream ss;
            ss << "Couldn't open snapshot file " << filename_ << strerror(errno);
            CrashReport::GenerateAndExit(ss);
        }
    }

    SnapshotFile::SnapshotFile():
        filename_(GetNewSnapshotFilename()),
        file_(NULL){
        CheckSnapshotDirectory();
        LOG(INFO) << "creating snapshot file: " << filename_;
        if((file_ = fopen(filename_.data(), "wb")) == NULL){
            std::stringstream ss;
            ss << "Couldn't create snapshot file " << filename_ << ": " << strerror(errno);
            CrashReport::GenerateAndExit(ss);
        }
    }

    SnapshotFile::~SnapshotFile(){
        if(GetFilePointer() != NULL) Close();
    }

    void SnapshotFile::WriteBytes(uint8_t* bytes, size_t size){
        CHECK_FILE_POINTER;
        size_t written_bytes;
        if((written_bytes = fwrite(bytes, sizeof(uint8_t), size, GetFilePointer())) != size)
            LOG(WARNING) << "could only write " << written_bytes << "/" << size << " bytes to snapshot file: " << GetFilename();
        Flush();
    }

    void SnapshotFile::WriteInt(uint32_t value){
        CHECK_FILE_POINTER;
        size_t written_bytes;
        if((written_bytes = fwrite(&value, sizeof(uint32_t), 1, GetFilePointer())) != 1)
            LOG(WARNING) << "could only write " << written_bytes << "/" << sizeof(uint32_t) << " bytes to snapshot file: " << GetFilename();
        Flush();
    }

    void SnapshotFile::WriteString(const std::string& value){
        WriteInt(value.length());
        WriteBytes((uint8_t*)value.data(), value.length());
    }

    void SnapshotFile::WriteMessage(google::protobuf::Message& msg){
        uint32_t size = msg.ByteSizeLong();
        WriteInt(size);

        uint8_t data[size];
        if(!msg.SerializeToArray(data, size)){
            LOG(WARNING) << "couldn't serialize message to snapshot file: " << GetFilename();
            return;
        }
        WriteBytes(data, size);
    }

    bool SnapshotFile::ReadBytes(uint8_t* bytes, size_t size){
        return fread(bytes, sizeof(uint8_t), size, GetFilePointer()) == size;
    }

    bool SnapshotFile::ReadMessage(google::protobuf::Message& msg){
        uint32_t num_bytes = ReadInt();
        uint8_t bytes[num_bytes];
        if(!ReadBytes(bytes, num_bytes)) return false;
        return msg.ParseFromArray(bytes, num_bytes);
    }

    uint8_t SnapshotFile::ReadByte(){
        uint8_t bytes[1];
        if(!ReadBytes(bytes, 1)){
            std::stringstream ss;
            ss << "Couldn't read byte from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return bytes[0];
    }

    uint32_t SnapshotFile::ReadInt(){
        uint8_t bytes[4];
        if(!ReadBytes(bytes, 4)){
            std::stringstream ss;
            ss << "Couldn't read int from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return (*(uint32_t*)bytes);
    }

    std::string SnapshotFile::ReadString(){
        uint32_t size = ReadInt();
        uint8_t bytes[size];
        if(!ReadBytes(bytes, size)){
            std::stringstream ss;
            ss << "Couldn't read string of size " << size << " from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return std::string((char*)bytes, size);
    }

    void SnapshotFile::Flush(){
        CHECK_FILE_POINTER;
        int err;
        if((err = fflush(GetFilePointer())) != 0)
            LOG(WARNING) << "couldn't flush snapshot file " << GetFilename() << ": " << strerror(err);
    }

    void SnapshotFile::Close(){
        CHECK_FILE_POINTER;
        int err;
        if((err = fclose(GetFilePointer())) != 0)
            LOG(WARNING) << "couldn't close snapshot file " << GetFilename() << ": " << strerror(err);
    }

    bool SnapshotWriter::WriteSnapshot(){
        SnapshotPrologueSection prologue;
        if(!prologue.Accept(this)){
            LOG(WARNING) << "couldn't write prologue section to snapshot: " << GetFile()->GetFilename();
            return false;
        }
        return true;
    }

    Snapshot* SnapshotReader::ReadSnapshot(){
        Snapshot* snapshot = new Snapshot();

        SnapshotPrologueSection prologue(snapshot);
        if(!prologue.Accept(this)){
            LOG(WARNING) << "couldn't read prologue section from snapshot: " << GetFile()->GetFilename();
            delete snapshot;
            return nullptr;
        }

        return snapshot;
    }
}