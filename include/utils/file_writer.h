#ifndef TOKEN_FILE_WRITER_H
#define TOKEN_FILE_WRITER_H

#include <iostream>
#include "object.h"
#include "version.h"

namespace Token{
  #define FOR_EACH_RAW_TYPE(V) \
    V(Byte, int8_t)            \
    V(Short, int16_t)          \
    V(Int, int32_t)            \
    V(Long, int64_t)

  class FileWriter{
   protected:
    std::string filename_;
    FileWriter* parent_;
    FILE* file_;

    FileWriter(const std::string& filename):
      filename_(filename),
      parent_(nullptr),
      file_(nullptr){}
    FileWriter(FileWriter* parent):
      filename_(parent->GetFilename()),
      parent_(parent),
      file_(parent->GetFilePointer()){}

    bool HasParent() const{
      return parent_ != nullptr;
    }

    bool HasFilePointer() const{
      return file_ != nullptr;
    }

    FILE* GetFilePointer() const{
      return file_;
    }
   public:
    virtual ~FileWriter(){
      if(HasFilePointer() && !HasParent()){
        Close();
      }
    }

    std::string GetFilename() const{
      return filename_;
    }

    #define DECLARE_WRITE(Name, Type) \
      bool Write##Name(const Type& val); \
      bool WriteUnsigned##Name(const u##Type& val);
    FOR_EACH_RAW_TYPE(DECLARE_WRITE)
    #undef DECLARE_WRITE

    int64_t GetCurrentPosition();
    bool SetCurrentPosition(int64_t pos);
    bool Flush();
    bool Close();
  };

  class TextFileWriter : public FileWriter{
   protected:
    size_t indent_;

    inline void Indent(){
      indent_++;
    }

    inline void DeIndent(){
      indent_--;
    }

    size_t GetIndent() const{
      return indent_;
    }
   public:
    TextFileWriter(const std::string& filename):
      FileWriter(filename),
      indent_(0){
      if((file_ = fopen(filename.c_str(), "w")) == NULL)
        LOG(WARNING) << "couldn't create text file " << filename << ": " << strerror(errno);
    }
    ~TextFileWriter() = default;

    bool Write(const std::string& value);
    bool Write(const Hash& hash);
    bool Write(Object* obj);
    bool NewLine();

    inline bool
    Write(const std::stringstream& ss){
      return Write(ss.str());
    }

    template<typename T>
    inline bool Write(T* value){
      return Write((Object*) value);
    }

    inline bool
    WriteLine(const std::string& value){
      return Write(value + '\n');
    }

    inline bool
    WriteLine(const std::stringstream& value){
      return Write(value.str() + '\n');
    }
  };

  class BinaryFileWriter : public FileWriter{
   protected:
    BinaryFileWriter(const std::string& filename):
      FileWriter(filename){
      if((file_ = fopen(filename.c_str(), "wb")) == NULL)
        LOG(WARNING) << "couldn't create binary file " << filename << ": " << strerror(errno);
    }
    BinaryFileWriter(BinaryFileWriter* parent):
      FileWriter(parent){}
   public:
    ~BinaryFileWriter() = default;

    bool WriteBytes(uint8_t* bytes, intptr_t size);
    bool WriteHash(const Hash& value);
    bool WriteUser(const User& user);
    bool WriteProduct(const Product& product);
    bool WriteString(const std::string& value);
    bool WriteObject(Object* obj);

    bool WriteVersion(const Version& version){
      WriteShort(version.GetMajor());
      WriteShort(version.GetMinor());
      WriteShort(version.GetRevision());
      return true;
    }

    bool WriteObjectTag(const ObjectTag& tag){
      return WriteUnsignedLong(tag.data());
    }

    bool WriteObjectTag(const ObjectTag::Type& tag){
      return WriteObjectTag(ObjectTag(tag));
    }

    template<typename T>
    inline bool WriteObject(T* value){
      return WriteObject((Object*) value);
    }

    template<class T>
    bool WriteList(const std::vector<T>& items){
      WriteLong((int64_t) items.size());
      for(auto& item : items){
        if(!item.Write(this)){
          return false;
        }
      }
      return true;
    }

    template<class T, class C>
    bool WriteSet(const std::set<T, C>& items){
      WriteLong((int64_t) items.size());
      for(auto& item : items){
        if(!item->Write(this)){
          return false;
        }
      }
      return true;
    }
  };

  class BinaryObjectFileWriter : public BinaryFileWriter{
   public:
    BinaryObjectFileWriter(const std::string& filename):
      BinaryFileWriter(filename){}
    virtual ~BinaryObjectFileWriter() = default;

    template<class T>
    bool WriteObject(const std::shared_ptr<T>& val){
      ObjectTag tag = val->tag();
      WriteObjectTag(tag);
      return val->Write(this);
    }
  };

  #undef FOR_EACH_RAW_TYPE
}

#endif //TOKEN_FILE_WRITER_H