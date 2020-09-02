#include "file_writer.h"
#include "bytes.h"
#include "bitfield.h"

namespace Token{
#define CHECK_FILE_POINTER \
    if(GetFilePointer() == NULL){ \
        LOG(WARNING) << "file " << GetFilename() << " is not opened"; \
        return false; \
    }
#define CHECK_WRITTEN(Written, Expected, Size) \
    if((Written) != (Expected)){ \
        LOG(WARNING) << "could only write " << ((Written)*(Size)) << "/" << ((Expected)*(Size)) << " bytes to file: " << GetFilename(); \
        return false; \
    }

    bool FileWriter::Close(){
        CHECK_FILE_POINTER;
        int err;
        if((err = fclose(GetFilePointer())) != 0){
            LOG(WARNING) << "couldn't close file " << GetFilename() << ": " << strerror(err);
            return false;
        }
        return true;
    }

    bool FileWriter::SetCurrentPosition(int64_t pos){
        CHECK_FILE_POINTER;
        int err;
        if((err = fseek(GetFilePointer(), pos, SEEK_SET)) != 0){
            LOG(WARNING) << "couldn't seek to " << pos << " in file " << GetFilename() << ": " << strerror(err);
            return false;
        }
        return true;
     }

    bool FileWriter::Flush(){
        CHECK_FILE_POINTER;
        int err;
        if((err = fflush(GetFilePointer())) != 0){
            LOG(WARNING) << "couldn't flush file " << GetFilename() << ": " << strerror(err);
            return false;
        }
        return true;
    }

    int64_t FileWriter::GetCurrentPosition(){
        if(GetFilePointer() == NULL){
            LOG(WARNING) << "file " << GetFilename() << " is not opened";
            return 0;
        }
        return ftell(GetFilePointer());
    }

    bool FileWriter::WriteBytes(uint8_t* bytes, size_t size){
        CHECK_FILE_POINTER;
        CHECK_WRITTEN(fwrite(bytes, sizeof(uint8_t), size, GetFilePointer()), size, sizeof(uint8_t));
        return Flush();
    }

    bool FileWriter::WriteInt(int32_t value){
        CHECK_FILE_POINTER;
        CHECK_WRITTEN(fwrite(&value, sizeof(int32_t), 1, GetFilePointer()), 1, sizeof(int32_t));
        return Flush();
    }

    bool FileWriter::WriteUnsignedInt(uint32_t value){
        CHECK_FILE_POINTER;
        CHECK_WRITTEN(fwrite(&value, sizeof(uint32_t), 1, GetFilePointer()), 1, sizeof(uint32_t));
        return Flush();
    }

    bool FileWriter::WriteLong(int64_t value){
        CHECK_FILE_POINTER;
        CHECK_WRITTEN(fwrite(&value, sizeof(int64_t), 1, GetFilePointer()), 1, sizeof(int64_t));
        return Flush();
    }

    bool FileWriter::WriteUnsignedLong(uint64_t value){
        CHECK_FILE_POINTER;
        CHECK_WRITTEN(fwrite(&value, sizeof(uint64_t), 1, GetFilePointer()), 1, sizeof(uint64_t));
        return Flush();
    }

    bool FileWriter::WriteHash(const uint256_t& hash){
        return WriteBytes((uint8_t*)hash.data(), uint256_t::kSize);
    }

    bool FileWriter::WriteString(const std::string& value){
        return WriteInt(value.length()) && WriteBytes((uint8_t*)value.data(), value.length());
    }

    bool BinaryFileWriter::WriteObject(Object* value){
        Type type = value->GetType();
        uint32_t size = value->GetBufferSize();

        ByteBuffer bytes(size);
        if(!value->Encode(&bytes)){
            LOG(WARNING) << "couldn't encode object: " << value->ToString();
            return false;
        }
        return WriteUnsignedLong(CreateObjectHeader(value)) && WriteBytes(&bytes);
    }
}