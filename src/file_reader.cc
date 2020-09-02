#include "file_reader.h"
#include "bytes.h"
#include "crash_report.h"

#include "block.h"

namespace Token{
#define CHECK_FILE_POINTER \
    if(GetFilePointer() == NULL){ \
        LOG(WARNING) << "file " << GetFilename() << " is not opened"; \
        return; \
    }
#define CHECK_WRITTEN(Written, Expected, Size) \
    if((Written) != (Expected)) \
        LOG(WARNING) << "could only write " << ((Written)*(Size)) << "/" << ((Expected)*(Size)) << " bytes to file: " << GetFilename();

    void FileReader::Close(){
        CHECK_FILE_POINTER;
        int err;
        if((err = fclose(GetFilePointer())) != 0)
            LOG(WARNING) << "couldn't close file " << GetFilename() << ": " << strerror(err);
    }

    int64_t FileReader::GetCurrentPosition(){
        if(GetFilePointer() == NULL){
            LOG(WARNING) << "file " << GetFilename() << " is not opened";
            return 0;
        }
        return ftell(GetFilePointer());
    }

    bool FileReader::ReadBytes(uint8_t* bytes, size_t size){
        //TODO: validate amount read
        size_t nread = fread(bytes, sizeof(uint8_t), size, GetFilePointer());
        return nread == size;
    }

    int32_t FileReader::ReadInt(){
        uint8_t bytes[4];
        if(!ReadBytes(bytes, 4)){
            std::stringstream ss;
            ss << "Couldn't read int from file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return (*(int32_t*)bytes);
    }

    uint32_t FileReader::ReadUnsignedInt(){
        uint8_t bytes[4];
        if(!ReadBytes(bytes, 4)){
            std::stringstream ss;
            ss << "Couldn't read unsigned int from file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return (*(uint32_t*)bytes);
    }

    int64_t FileReader::ReadLong(){
        uint8_t bytes[8];
        if(!ReadBytes(bytes, 8)){
            std::stringstream ss;
            ss << "Couldn't read long from file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return (*(int64_t*)bytes);
    }

    uint64_t FileReader::ReadUnsignedLong(){
        uint8_t bytes[8];
        if(!ReadBytes(bytes, 8)){
            std::stringstream ss;
            ss << "Couldn't read unsigned long from file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return (*(uint64_t*)bytes);
    }

    uint256_t FileReader::ReadHash(){
        uint8_t bytes[uint256_t::kSize];
        if(!ReadBytes(bytes, uint256_t::kSize)){
            std::stringstream ss;
            ss << "Couldn't read hash from file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return uint256_t((uint8_t*)bytes);
    }

    std::string FileReader::ReadString(){
        int32_t size = ReadInt();
        uint8_t bytes[size];
        if(!ReadBytes(bytes, size)){
            std::stringstream ss;
            ss << "Couldn't read string of size " << size << " from file: " << GetFilename();
            CrashReport::GenerateAndExit(ss);
        }
        return std::string((char*)bytes, size);
    }

    void FileReader::SetCurrentPosition(int64_t pos){
        CHECK_FILE_POINTER;
        int err;
        if((err = fseek(GetFilePointer(), pos, SEEK_SET)) != 0)
            LOG(WARNING) << "couldn't seek to " << pos << " in file " << GetFilename() << ": " << strerror(err);
    }

    Handle<Object> BinaryFileReader::ReadObject(){
        ObjectHeader header = ReadUnsignedLong();
        Type type = GetObjectHeaderType(header);
        uint32_t size = GetObjectHeaderSize(header);

        LOG(INFO) << "reading object of type: " << type << " (" << size << " Bytes) @" << GetCurrentPosition();
        ByteBuffer bytes(size);
        if(!ReadBytes(&bytes, size)){
            LOG(WARNING) << "couldn't read object";
            return nullptr;
        }

        switch(GetObjectHeaderType(header)){
#define DEFINE_DECODE(Name) \
            case Type::k##Name##Type: return Name::NewInstance(&bytes).CastTo<Object>();
            FOR_EACH_TYPE(DEFINE_DECODE);
#undef DEFINE_DECODE
            case Type::kUnknownType:
            default:
                LOG(WARNING) << "unknown object type: " << type;
                return nullptr;
        }
    }
}