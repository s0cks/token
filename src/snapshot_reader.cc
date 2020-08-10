#include "snapshot_reader.h"

namespace Token{
#define CHECK_FILE_POINTER \
    if(GetFilePointer() == NULL){ \
        LOG(WARNING) << "snapshot file " << GetFilename() << " is not opened"; \
        return; \
    }
#define CHECK_WRITTEN(Written, Expected, Size) \
    if((Written) != (Expected)) \
        LOG(WARNING) << "could only write " << ((Written)*(Size)) << "/" << ((Expected)*(Size)) << " bytes to snapshot file: " << GetFilename();

    void SnapshotReader::Close(){
        CHECK_FILE_POINTER;
        int err;
        if((err = fclose(GetFilePointer())) != 0)
            LOG(WARNING) << "couldn't close snapshot file " << GetFilename() << ": " << strerror(err);
    }

    int64_t SnapshotReader::GetCurrentPosition(){
        if(GetFilePointer() == NULL){
            LOG(WARNING) << "snapshot file " << GetFilename() << " is not opened";
            return 0;
        }
        return ftell(GetFilePointer());
    }

    bool SnapshotReader::ReadBytes(uint8_t* bytes, size_t size){
        //TODO: validate amount read
        return fread(bytes, sizeof(uint8_t), size, GetFilePointer()) == size;
    }

    int32_t SnapshotReader::ReadInt(){
        uint8_t bytes[4];
        if(!ReadBytes(bytes, 4)){
            std::stringstream ss;
            ss << "Couldn't read int from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return (*(int32_t*)bytes);
    }

    uint32_t SnapshotReader::ReadUnsignedInt(){
        uint8_t bytes[4];
        if(!ReadBytes(bytes, 4)){
            std::stringstream ss;
            ss << "Couldn't read unsigned int from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return (*(uint32_t*)bytes);
    }

    int64_t SnapshotReader::ReadLong(){
        uint8_t bytes[8];
        if(!ReadBytes(bytes, 8)){
            std::stringstream ss;
            ss << "Couldn't read long from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return (*(int64_t*)bytes);
    }

    uint64_t SnapshotReader::ReadUnsignedLong(){
        uint8_t bytes[8];
        if(!ReadBytes(bytes, 8)){
            std::stringstream ss;
            ss << "Couldn't read unsigned long from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return (*(uint64_t*)bytes);
    }

    IndexReference SnapshotReader::ReadReference(){
        uint256_t hash = ReadHash();
        size_t size = ReadInt();
        int64_t index_pos = ReadLong();
        int64_t data_pos = ReadLong();
        return IndexReference(hash, size, index_pos, data_pos);
    }

    uint256_t SnapshotReader::ReadHash(){
        uint8_t bytes[uint256_t::kSize];
        if(!ReadBytes(bytes, uint256_t::kSize)){
            std::stringstream ss;
            ss << "Couldn't read hash from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return uint256_t((uint8_t*)bytes);//TODO: fix sign-casting
    }

    std::string SnapshotReader::ReadString(){
        int32_t size = ReadInt();
        uint8_t bytes[size];
        if(!ReadBytes(bytes, size)){
            std::stringstream ss;
            ss << "Couldn't read string of size " << size << " from snapshot file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return std::string((char*)bytes, size);
    }

    void SnapshotReader::SetCurrentPosition(int64_t pos){
        CHECK_FILE_POINTER;
        int err;
        if((err = fseek(GetFilePointer(), pos, SEEK_SET)) != 0)
            LOG(WARNING) << "couldn't seek to " << pos << " in snapshot " << GetFilename() << ": " << strerror(err);
    }

    Snapshot* SnapshotReader::ReadSnapshot(){
        Snapshot* snapshot = new Snapshot();
        snapshot->filename_ = GetFilename();
        snapshot->prologue_.Accept(this);
        snapshot->blocks_.Accept(this);
        snapshot->utxos_.Accept(this);
        return snapshot;
    }
}