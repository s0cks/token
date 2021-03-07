#ifndef TOKEN_UTILS_FILESYSTEM_H
#define TOKEN_UTILS_FILESYSTEM_H

#include <cstdio>
#include "buffer.h"

namespace token{
  static inline bool
  Flush(FILE* file){
    //TODO: better logging
    if(file == NULL){
#ifdef TOKEN_DEBUG
      LOG(WARNING) << "cannot flush file, file is not open.";
#endif//TOKEN_DEBUG
      return false;
    }

    if(fflush(file) != 0){
      LOG(ERROR) << "cannot flush file: " << strerror(errno);
      return false;
    }

#ifdef TOKEN_DEBUG
    LOG(INFO) << "file flushed.";
#endif//TOKEN_DEBUG
    return true;
  }

  static inline bool
  Close(FILE* file){
    if(file == NULL){
      //TODO: better logging
#ifdef TOKEN_DEBUG
      LOG(WARNING) << "cannot close file, file is not open.";
#endif//TOKEN_DEBUG
      return false; //TODO: should we return true for this case?
    }

    if(fclose(file) != 0){
      LOG(ERROR) << "cannot close file: " << strerror(errno);
      return false;
    }

#ifdef TOKEN_DEBUG
    LOG(INFO) << "file closed.";
#endif//TOKEN_DEBUG
    return true;
  }

  static inline bool
  Seek(FILE* file, const int64_t pos, int whence=SEEK_SET){
    if(file == NULL){
#ifdef TOKEN_DEBUG
      LOG(WARNING) << "cannot seek file, file is not open.";
#endif//TOKEN_DEBUG
      return false;
    }

    if(fseek(file, pos, whence) != 0){
      LOG(ERROR) << "cannot seek to position " << pos << " in file.";
      return false;
    }
    return true;
  }

  bool ReadBytes(FILE* file, uint8_t* data, const int64_t& nbytes);

  inline bool
  ReadBytes(FILE* file, const BufferPtr& buffer, const int64_t nbytes){
    return ReadBytes(file, (uint8_t*)buffer->data(), nbytes);
  }

  inline bool
  ReadBytes(FILE* file, const BufferPtr& buffer){
    return ReadBytes(file, (uint8_t*)buffer->data(), buffer->GetBufferSize());
  }

#define DECLARE_READ_SIGNED(Name, Type) \
  Type Read##Name(FILE* file);
#define DECLARE_READ_UNSIGNED(Name, Type) \
  u##Type ReadUnsigned##Name(FILE* file);
#define DECLARE_READ(Name, Type) \
  DECLARE_READ_SIGNED(Name, Type)\
  DECLARE_READ_UNSIGNED(Name, Type)

  FOR_EACH_RAW_TYPE(DECLARE_READ)
#undef DECLARE_READ
#undef DECLARE_READ_SIGNED
#undef DECLARE_READ_UNSIGNED

  static inline ObjectTag
  ReadTag(FILE* file){
    return ObjectTag(ReadUnsignedLong(file));
  }

  static inline Hash
  ReadHash(FILE* file){
    uint8_t data[Hash::GetSize()];
    if(!ReadBytes(file, data, Hash::GetSize()))
      return Hash();
    return Hash(data);
  }

  static inline Version
  ReadVersion(FILE* file){
    return Version(ReadUnsignedLong(file));
  }

  static inline Timestamp
  ReadTimestamp(FILE* file){
    return FromUnixTimestamp(ReadUnsignedLong(file));
  }

  template<typename T, typename C>
  static inline bool
  ReadSet(FILE* file, const Type& type, std::set<std::shared_ptr<T>, C>& data){
    int64_t length = ReadLong(file);
    for(int64_t idx = 0; idx < length; idx++){
      ObjectTag tag = ReadTag(file);
      if(!tag.IsValid()){
        LOG(WARNING) << "#" << idx << " has an invalid tag: " << tag;
        return false;
      }

      if(tag.GetType() != type){
        LOG(WARNING) << "#" << idx << " (" << tag << ") is not a " << type;
        return false;
      }

      BufferPtr buffer = Buffer::NewInstance(tag.GetSize());
      if(!ReadBytes(file, buffer, tag.GetSize())){
        LOG(WARNING) << "#" << idx << " is invalid.";
        return false;
      }

      std::shared_ptr<T> val = T::FromBytes(buffer);
      if(!data.insert(val).second){
        LOG(WARNING) << "#" << idx << " is invalid.";
        return false;
      }
    }
    return data.size() == (size_t)length;
  }

  bool WriteBytes(FILE* file, const uint8_t* data, const int64_t& nbytes);

  inline bool
  WriteBytes(FILE* file, const BufferPtr& buffer, const int64_t& nbytes){
    return WriteBytes(file, (const uint8_t*)buffer->data(), nbytes);
  }

  inline bool
  WriteBytes(FILE* file, const BufferPtr& buffer){
    return WriteBytes(file, (const uint8_t*)buffer->data(), buffer->GetWrittenBytes());
  }

#define DECLARE_WRITE_SIGNED(Name, Type) \
  bool Write##Name(FILE* file, const Type& val);
#define DECLARE_WRITE_UNSIGNED(Name, Type) \
  bool Write##Unsigned##Name(FILE* file, const u##Type& val);
#define DECLARE_WRITE(Name, Type) \
  DECLARE_WRITE_SIGNED(Name, Type)\
  DECLARE_WRITE_UNSIGNED(Name, Type)

  FOR_EACH_RAW_TYPE(DECLARE_WRITE)
#undef DECLARE_WRITE
#undef DECLARE_WRITE_SIGNED
#undef DECLARE_WRITE_UNSIGNED

#define DECLARE_WRITE(Name) \
  static inline bool Write##Name(FILE* file, const Name& val){ return WriteBytes(file, (const uint8_t*)val.data(), val.size()); }
  FOR_EACH_SERIALIZABLE_TYPE(DECLARE_WRITE)
#undef DECLARE_WRITE

  static inline bool
  WriteTag(FILE* file, const ObjectTag& tag){
    return WriteUnsignedLong(file, tag.raw());
  }

  static inline bool
  WriteVersion(FILE* file, const Version& version){
    return WriteUnsignedLong(file, version.raw());
  }

  static inline bool
  WriteTimestamp(FILE* file, const Timestamp& timestamp){
    return WriteUnsignedLong(file, ToUnixTimestamp(timestamp));
  }

  template<typename T, typename C>
  static inline bool
  WriteSet(FILE* file, const std::set<T, C>& val){
    int64_t size = val.size();
    if(!WriteLong(file, size)){
      LOG(WARNING) << "cannot write set size to file.";
      return false;
    }

    size_t idx = 0;
    for(auto& it : val){
      ++idx;

      ObjectTag tag = it->GetTag();
      if(!WriteTag(file, tag)){
        LOG(WARNING) << "cannot write tag for #" << (idx) << " to file.";
        return false;
      }

      BufferPtr buffer = it->ToBuffer();
      if(!WriteBytes(file, buffer)){
        LOG(WARNING) << "cannot write object #" << (idx) << " in set to file.";
        return false;
      }
    }
    return true;
  }
}

#endif//TOKEN_UTILS_FILESYSTEM_H