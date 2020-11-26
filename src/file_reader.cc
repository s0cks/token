#include "file_reader.h"
#include "crash_report.h"

#include "buffer.h"
#include "block.h"
#include "vmemory.h"

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
        if(nread != size) LOG(WARNING) << "read " << nread << "/" << size << " bytes";
        return nread == size;
    }

    int32_t FileReader::ReadInt(){
        uint8_t bytes[4];
        if(!ReadBytes(bytes, 4)){
            LOG(WARNING) << "couldn't read int from file: " << GetFilename();
            return 0;
        }
        return (*(int32_t*)bytes);
    }

    uint32_t FileReader::ReadUnsignedInt(){
        uint8_t bytes[4];
        if(!ReadBytes(bytes, 4)){
            LOG(WARNING) << "couldn't read unsigned int from file: " << GetFilename();
            return 0;
        }
        return (*(uint32_t*)bytes);
    }

    int64_t FileReader::ReadLong(){
        uint8_t bytes[8];
        if(!ReadBytes(bytes, 8)){
            LOG(WARNING) << "couldn't read long from file: " << GetFilename();
            return 0;
        }
        return (*(int64_t*)bytes);
    }

    uint64_t FileReader::ReadUnsignedLong(){
        uint8_t bytes[8];
        if(!ReadBytes(bytes, 8)){
            LOG(WARNING) << "couldn't read unsigned long from file: " << GetFilename();
            return 0;
        }
        return (*(uint64_t*)bytes);
    }

    Hash FileReader::ReadHash(){
        uint8_t bytes[Hash::kSize];
        if(!ReadBytes(bytes, Hash::kSize)){
            LOG(WARNING) << "couldn't read Hash from file: " << GetFilename();
            return Hash();
        }

        return Hash::FromBytes(bytes);
    }

    std::string FileReader::ReadString(){
        int32_t size = ReadInt();
        uint8_t bytes[size];
        if(!ReadBytes(bytes, size)){
            LOG(WARNING) << "couldn't read string of size " << size << " from file: " << GetFilename();
            return "";
        }
        return std::string((char*)bytes, size);
    }

    void FileReader::SetCurrentPosition(int64_t pos){
        CHECK_FILE_POINTER;
        int err;
        if((err = fseek(GetFilePointer(), pos, SEEK_SET)) != 0)
            LOG(WARNING) << "couldn't seek to " << pos << " in file " << GetFilename() << ": " << strerror(err);
    }

    bool BinaryFileReader::ReadRegion(MemoryRegion* region, intptr_t nbytes){
        return ReadBytes((uint8_t*)region->GetPointer(), nbytes);
    }
}