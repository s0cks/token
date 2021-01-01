#include "utils/file_reader.h"
#include "utils/crash_report.h"

#include "utils/buffer.h"
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
    if(nread != size) LOG(WARNING) << "read " << nread << "/" << size << " bytes";
    return nread == size;
  }

  std::string FileReader::ReadString(){
    int32_t size = ReadInt();
    uint8_t bytes[size];
    if(!ReadBytes(bytes, size)){
      LOG(WARNING) << "couldn't read string of size " << size << " from file: " << GetFilename();
      return "";
    }
    return std::string((char*) bytes, size);
  }

  void FileReader::SetCurrentPosition(int64_t pos){
    CHECK_FILE_POINTER;
    int err;
    if((err = fseek(GetFilePointer(), pos, SEEK_SET)) != 0)
      LOG(WARNING) << "couldn't seek to " << pos << " in file " << GetFilename() << ": " << strerror(err);
  }
}