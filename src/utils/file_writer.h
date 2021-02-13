#ifndef TOKEN_UTILS_FILE_WRITER_H
#define TOKEN_UTILS_FILE_WRITER_H

#include <set>
#include <vector>
#include <unordered_set>
#include <cstdio>
#include <glog/logging.h>
#include "object.h"
#include "utils/buffer.h"

namespace token{
  class FileWriter{
   protected:
    std::string filename_;
    FILE* file_;

    FileWriter(const std::string& filename):
      filename_(filename),
      file_(NULL){
      if((file_ = fopen(GetFilename(), "wb")) == NULL)
        LOG(WARNING) << "cannot open " << GetFilename() << ": " << strerror(errno);
    }
   public:
    virtual ~FileWriter(){
      if(IsOpen() && !(Flush() && Close()))
        LOG(ERROR) << GetFilename() << " is not closed.";
    }

    const char* GetFilename() const{
      return filename_.data();
    }

    bool IsOpen() const{
      return file_ != NULL;
    }

    bool WriteBytes(uint8_t* bytes, const int64_t& size) const;

#define DECLARE_WRITE_UNSIGNED(Name, Type) \
    bool WriteUnsigned##Name(const u##Type& val) const;
#define DECLARE_WRITE_SIGNED(Name, Type) \
    bool Write##Name(const Type& val) const;
#define DECLARE_WRITE(Name, Type) \
    DECLARE_WRITE_UNSIGNED(Name, Type) \
    DECLARE_WRITE_SIGNED(Name, Type)

    FOR_EACH_RAW_TYPE(DECLARE_WRITE)
#undef DECLARE_WRITE
#undef DECLARE_WRITE_SIGNED
#undef DECLARE_WRITE_UNSIGNED

#define DECLARE_WRITE(Name) \
    bool Write##Name(const Name& val) const{ return WriteBytes(val.raw_data(), val.size()); }
    FOR_EACH_SERIALIZABLE_TYPE(DECLARE_WRITE)
#undef DECLARE_WRITE

    template<typename T, typename C>
    bool WriteSet(const std::set<T, C>& val) const{
      int64_t size = val.size();
      if(!WriteLong(size)){
        LOG(WARNING) << "cannot write set size";
        return false;
      }

      for(auto& it : val){
        BufferPtr buffer = it->ToBufferTagged();
        if(!WriteBytes(buffer))
          return false;
      }
      return true;
    }

    inline bool
    WriteBytes(const BufferPtr& buffer) const{
      return WriteBytes((uint8_t*)buffer->data(), buffer->GetWrittenBytes());
    }

    bool Flush() const{
      if(!IsOpen()){
        LOG(WARNING) << "cannot flush " << GetFilename() << ", file is not open.";
        return false;
      }

      if(fflush(file_) != 0){
        LOG(ERROR) << "cannot flush " << GetFilename() << ": " << strerror(errno);
        return false;
      }
      LOG(INFO) << GetFilename() << " flushed.";
      return true;
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
      LOG(INFO) << GetFilename() << " closed.";
      return true;
    }
  };

  class TaggedFileWriter : public FileWriter{
   protected:
    ObjectTag tag_;

    TaggedFileWriter(const std::string& filename, const ObjectTag& tag):
      FileWriter(filename),
      tag_(tag){}
   public:
    virtual ~TaggedFileWriter() = default;

    ObjectTag& tag(){
      return tag_;
    }

    ObjectTag tag() const{
      return tag_;
    }

    bool WriteTag() const{
      return WriteUnsignedLong(tag_.raw());
    }
  };
}

#endif//TOKEN_UTILS_FILE_WRITER_H