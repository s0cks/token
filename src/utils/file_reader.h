#ifndef TOKEN_UTILS_FILE_READER_H
#define TOKEN_UTILS_FILE_READER_H

#include <cstdio>
#include <glog/logging.h>
#include "object.h"

namespace token{
  class FileReader{
   protected:
    std::string filename_;
    FILE* file_;

    FileReader(const std::string& filename):
      filename_(filename),
      file_(NULL){
      if((file_ = fopen(GetFilename(), "rb")) == NULL)
        LOG(WARNING) << "cannot open " << GetFilename() << ": " << strerror(errno);
    }
   public:
    virtual ~FileReader(){
      if(IsOpen() && !Close()){
        LOG(WARNING) << "cannot close file " << GetFilename();
      }
    }

    const char* GetFilename() const{
      return filename_.data();
    }

    bool ReadBytes(uint8_t* data, const int64_t& size) const;

#define DECLARE_READ_SIGNED(Name, Type) \
    Type Read##Name() const;
#define DECLARE_READ_UNSIGNED(Name, Type) \
    u##Type ReadUnsigned##Name() const;
#define DECLARE_READ(Name, Type) \
    DECLARE_READ_SIGNED(Name, Type) \
    DECLARE_READ_UNSIGNED(Name, Type)

    FOR_EACH_RAW_TYPE(DECLARE_READ)
#undef DECLARE_READ
#undef DECLARE_READ_SIGNED
#undef DECLARE_READ_UNSIGNED

    bool Seek(const int64_t pos, int whence=SEEK_SET) const{
      if(fseek(file_, pos, whence) != 0){
        LOG(WARNING) << "cannot set current position to " << pos << " in file " << GetFilename();
        return false;
      }
      return true;
    }

    bool IsOpen() const{
      return file_ != NULL;
    }

    bool Close() const{
      if(!IsOpen()){
        LOG(WARNING) << "cannot close " << GetFilename() << ", file is not open.";
        return false;
      }

      if(fclose(file_) != 0){
        LOG(ERROR) << "cannot close " << GetFilename() << ": " << strerror(errno);
        return false;
      }
      return true;
    }
  };

  class TaggedFileReader : public FileReader{
   protected:
    TaggedFileReader(const std::string& filename):
      FileReader(filename){}
   public:
    virtual ~TaggedFileReader() = default;

    ObjectTag ReadTag() const{
      return ObjectTag(ReadUnsignedLong());
    }

    bool CheckTag(const Type& type) const{
      ObjectTag tag = ReadTag();
      if(!tag.IsValid()){
        LOG(WARNING) << "tag " << tag << " is not valid.";
        return false;
      }

      if(tag.GetType() != type){
        LOG(WARNING) << "tag " << tag << " is not of type " << type << ".";
        return false;
      }
      return true;
    }
  };
}

#endif//TOKEN_UTILS_FILE_READER_H