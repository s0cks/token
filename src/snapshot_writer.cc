#include "snapshot_writer.h"

namespace Token{
#define CHECK_FILE_POINTER \
    if(GetFilePointer() == NULL){ \
        LOG(WARNING) << "snapshot file " << GetFilename() << " is not opened"; \
        return; \
    }
#define CHECK_WRITTEN(Written, Expected, Size) \
    if((Written) != (Expected)) \
        LOG(WARNING) << "could only write " << ((Written)*(Size)) << "/" << ((Expected)*(Size)) << " bytes to snapshot file: " << GetFilename();

    void SnapshotWriter::Close(){
        CHECK_FILE_POINTER;
        int err;
        if((err = fclose(GetFilePointer())) != 0)
            LOG(WARNING) << "couldn't close snapshot file " << GetFilename() << ": " << strerror(err);
    }

    void SnapshotWriter::SetCurrentPosition(int64_t pos){
        CHECK_FILE_POINTER;
        int err;
        if((err = fseek(GetFilePointer(), pos, SEEK_SET)) != 0)
            LOG(WARNING) << "couldn't seek to " << pos << " in snapshot " << GetFilename() << ": " << strerror(err);
    }

    void SnapshotWriter::Flush(){
        CHECK_FILE_POINTER;
        int err;
        if((err = fflush(GetFilePointer())) != 0)
            LOG(WARNING) << "couldn't flush snapshot file " << GetFilename() << ": " << strerror(err);
    }

    int64_t SnapshotWriter::GetCurrentPosition(){
        if(GetFilePointer() == NULL){
            LOG(WARNING) << "snapshot file " << GetFilename() << " is not opened";
            return 0;
        }
        return ftell(GetFilePointer());
    }

    bool SnapshotWriter::WriteSnapshot(){
        Snapshot* snapshot = GetSnapshot();
        snapshot->GetPrologueSection().Accept(this); //TODO: check result
        snapshot->GetBlockChainSection().Accept(this); //TODO: check result
        return true;
    }

    void SnapshotWriter::WriteBytes(uint8_t* bytes, size_t size){
        CHECK_FILE_POINTER;
        CHECK_WRITTEN(fwrite(bytes, sizeof(uint8_t), size, GetFilePointer()), size, sizeof(uint8_t));
        Flush();
    }

    void SnapshotWriter::WriteInt(int32_t value){
        CHECK_FILE_POINTER;
        CHECK_WRITTEN(fwrite(&value, sizeof(int32_t), 1, GetFilePointer()), 1, sizeof(int32_t));
        Flush();
    }

    void SnapshotWriter::WriteUnsignedInt(uint32_t value){
        CHECK_FILE_POINTER;
        CHECK_WRITTEN(fwrite(&value, sizeof(uint32_t), 1, GetFilePointer()), 1, sizeof(uint32_t));
        Flush();
    }

    void SnapshotWriter::WriteLong(int64_t value){
        CHECK_FILE_POINTER;
        CHECK_WRITTEN(fwrite(&value, sizeof(int64_t), 1, GetFilePointer()), 1, sizeof(int64_t));
        Flush();
    }

    void SnapshotWriter::WriteUnsignedLong(uint64_t value){
        CHECK_FILE_POINTER;
        CHECK_WRITTEN(fwrite(&value, sizeof(uint64_t), 1, GetFilePointer()), 1, sizeof(uint64_t));
        Flush();
    }

    void SnapshotWriter::WriteHash(const uint256_t& hash){
        WriteBytes((uint8_t*)hash.data(), uint256_t::kSize);
    }

    void SnapshotWriter::WriteString(const std::string& value){
        WriteInt(value.length());
        WriteBytes((uint8_t*)value.data(), value.length());
    }

    void SnapshotWriter::WriteMessage(google::protobuf::Message& msg){
        int32_t size = msg.ByteSizeLong();
        WriteInt(size);

        uint8_t data[size];
        if(!msg.SerializeToArray(data, size)){
            LOG(WARNING) << "couldn't serialize message to snapshot file: " << GetFilename();
            return;
        }
        WriteBytes(data, size);
    }

    void SnapshotWriter::WriteReference(const IndexReference& ref){
        WriteHash(ref.GetHash());
        WriteInt(ref.GetSize());
        WriteLong(ref.GetIndexPosition());
        WriteLong(ref.GetDataPosition());
    }
}