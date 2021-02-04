#ifndef TOKEN_FILE_READER_H
#define TOKEN_FILE_READER_H

#include "hash.h"
#include "object.h"
#include "version.h"

namespace token{
  #define FOR_EACH_RAW_TYPE(V) \
    V(Byte, int8_t)   \
    V(Short, int16_t) \
    V(Int, int32_t)   \
    V(Long, int64_t)

  class FileReader{
   protected:
    FileReader* parent_;
    std::string filename_;
    FILE* file_;

    FileReader(const std::string& filename):
      parent_(nullptr),
      filename_(filename),
      file_(nullptr){}
    FileReader(FileReader* parent):
      parent_(parent),
      filename_(parent->filename_),
      file_(parent->file_){}

    bool HasParent() const{
      return parent_ != nullptr;
    }

    bool HasFilePointer() const{
      return file_ != nullptr;
    }

    FILE* GetFilePointer() const{
      return file_;
    }

    int64_t GetCurrentPosition();
    void SetCurrentPosition(int64_t pos);

    template<typename T>
    T Read(){
      uint8_t bytes[sizeof(T)];
      if(!ReadBytes(bytes, sizeof(T))){
        LOG(WARNING) << "couldn't read " << sizeof(T) << " bytes from file: " << GetFilename();
        return (T) 0;
      }
      return (*(T*) bytes);
    }

    template<typename T>
    T ReadRaw(){
      uint8_t bytes[T::GetSize()];
      if(!ReadBytes(bytes, T::GetSize())){
        LOG(WARNING) << "couldn't read " << T::GetSize() << " bytes from file: " << GetFilename();
        return T();
      }
      return T(bytes, T::GetSize());
    }
   public:
    virtual ~FileReader(){
      if(HasFilePointer() && !HasParent()){
        Close();
      }
    }

    std::string GetFilename() const{
      return filename_;
    }

    Hash ReadHash(){
      return ReadRaw<Hash>();
    }

    User ReadUser(){
      return ReadRaw<User>();
    }

    Product ReadProduct(){
      return ReadRaw<Product>();
    }

    Version ReadVersion(){
      return Version(ReadShort(), ReadShort(), ReadShort());
    }

    std::string ReadString();
    bool ReadBytes(uint8_t* bytes, size_t size);

    ObjectTag ReadObjectTag(){
      return ObjectTag(ReadUnsignedLong());
    }

    #define DEFINE_READ(Name, Type) \
      Type Read##Name(){ return Read<Type>(); } \
      u##Type Read##Unsigned##Name(){ return Read<u##Type>(); }
    FOR_EACH_RAW_TYPE(DEFINE_READ)
    #undef DEFINE_READ

    void Close();
  };

  class BinaryFileReader : public FileReader{
   protected:
    BinaryFileReader(const std::string& filename):
      FileReader(filename){
      if((file_ = fopen(filename.c_str(), "rb")) == NULL)
        LOG(WARNING) << "couldn't open binary file " << filename << ": " << strerror(errno);
    }
    BinaryFileReader(BinaryFileReader* parent):
      FileReader(parent){}
   public:
    virtual ~BinaryFileReader() = default;
  };

  class BinaryObjectFileReader : public BinaryFileReader{
   protected:
    Type type_;
   public:
    BinaryObjectFileReader(const std::string& filename, const Type& tag_type):
      BinaryFileReader(filename),
      type_(tag_type){}
    virtual ~BinaryObjectFileReader() = default;

    bool ValidateTag(){
      ObjectTag tag = ReadObjectTag();
      if(tag.GetType() != type_){
        LOG(WARNING) << "object tag of " << tag << " != " << type_;
        return false;
      }
      return true;
    }
  };

  #undef FOR_EACH_RAW_TYPE
}

#endif //TOKEN_FILE_READER_H
