#include "utils/bitfield.h"
#include "utils/file_writer.h"

namespace token{
  #define FOR_EACH_RAW_TYPE(V) \
    V(Byte, int8_t)            \
    V(Short, int16_t)          \
    V(Int, int32_t)            \
    V(Long, int64_t)

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

#define DEFINE_WRITE(Name, Type) \
  bool FileWriter::Write##Name(const Type& val){ \
    CHECK_FILE_POINTER; \
    CHECK_WRITTEN(fwrite(&val, sizeof(Type), 1, GetFilePointer()), 1, sizeof(Type)); \
    return Flush(); \
  } \
  bool FileWriter::WriteUnsigned##Name(const u##Type& val){ \
    CHECK_FILE_POINTER; \
    CHECK_WRITTEN(fwrite(&val, sizeof(u##Type), 1, GetFilePointer()), 1, sizeof(u##Type)); \
    return Flush(); \
  }
  FOR_EACH_RAW_TYPE(DEFINE_WRITE)
#undef DEFINE_WRITE

  bool FileWriter::Close(){
    CHECK_FILE_POINTER;
    int err;
    if((err = fclose(GetFilePointer())) != 0){
      LOG(WARNING) << "couldn't close file " << GetFilename() << ": " << strerror(err);
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

  bool FileWriter::SetCurrentPosition(int64_t pos){
    CHECK_FILE_POINTER;
    int err;
    if((err = fseek(GetFilePointer(), pos, SEEK_SET)) != 0){
      LOG(WARNING) << "couldn't seek to " << pos << " in file " << GetFilename() << ": " << strerror(err);
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

  bool TextFileWriter::Write(const std::string& value){
    CHECK_FILE_POINTER;

    std::stringstream ss;
    for(size_t idx = 0; idx < GetIndent(); idx++)
      ss << ' ';
    ss << value;

    std::string line = ss.str();
    CHECK_WRITTEN(fwrite(value.data(), sizeof(char), line.length(), GetFilePointer()), line.length(), sizeof(char));
    return Flush();
  }

  bool TextFileWriter::Write(const Hash& hash){
    return Write(hash.HexString());
  }

  bool TextFileWriter::Write(Object* obj){
    return Write(obj->ToString());
  }

  bool TextFileWriter::NewLine(){
    return Write((const std::string&) "\n");
  }

  bool BinaryFileWriter::WriteBytes(uint8_t* bytes, intptr_t size){
    CHECK_FILE_POINTER;
    CHECK_WRITTEN(fwrite(bytes, sizeof(uint8_t), (size_t) size, GetFilePointer()), (size_t) size, sizeof(uint8_t));
    return Flush();
  }

  bool BinaryFileWriter::WriteHash(const Hash& hash){
    return WriteBytes((uint8_t*) hash.data(), Hash::GetSize());
  }

  bool BinaryFileWriter::WriteUser(const User& user){
    return WriteBytes((uint8_t*) user.data(), User::GetSize());
  }

  bool BinaryFileWriter::WriteProduct(const Product& product){
    return WriteBytes((uint8_t*) product.data(), Product::GetSize());
  }

  bool BinaryFileWriter::WriteString(const std::string& value){
    return WriteInt(value.length()) && WriteBytes((uint8_t*) value.data(), value.length());
  }

  bool BinaryFileWriter::WriteObject(Object* value){
    LOG(WARNING) << "BinaryFileWriter::WriteObject(Object*) - not implemented yet!";
    return false; //TODO: implement BinaryFileWriter::WriteObject(Object*)
  }
}