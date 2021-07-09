#ifndef TOKEN_UTILS_FILESYSTEM_H
#define TOKEN_UTILS_FILESYSTEM_H

#include <cstdio>

#include "type.h"
#include "hash.h"
#include "version.h"
#include "timestamp.h"

namespace token{
namespace internal{
static inline bool
Flush(FILE* file){
  if(!file){
    DLOG(WARNING) << "file is null.";
    return false;
  }
  if(fflush(file) != 0){
    DLOG(ERROR) << "cannot flush file: " << strerror(errno);
    return false;
  }
  return true;
}

static inline bool
Close(FILE* file){
  if(!file){
    DLOG(WARNING) << "cannot close file, file is null.";
    return false; //should we return true for this case?
  }

  if(fclose(file) != 0){
    DLOG(ERROR) << "cannot close file: " << strerror(errno);
    return false;
  }
  return true;
}

static inline bool
Seek(FILE* file, const int64_t pos, int whence=SEEK_SET){
  if(!file){
    DLOG(WARNING) << "cannot seek file, file is not open.";
    return false;
  }

  if(fseek(file, pos, whence) != 0){
    DLOG(ERROR) << "cannot seek to " << pos << " in file: " << strerror(errno);
    return false;
  }
  return true;
}
}

  bool ReadBytes(FILE* file, uint8_t* data, const int64_t& nbytes);

#define DECLARE_READ_SIGNED(Name, Type) \
  Type##_t Read##Name(FILE* file);
#define DECLARE_READ_UNSIGNED(Name, Type) \
  u##Type##_t ReadUnsigned##Name(FILE* file);
#define DECLARE_READ(Name, Type) \
  DECLARE_READ_SIGNED(Name, Type)\
  DECLARE_READ_UNSIGNED(Name, Type)

  FOR_EACH_NATIVE_TYPE(DECLARE_READ)
#undef DECLARE_READ
#undef DECLARE_READ_SIGNED
#undef DECLARE_READ_UNSIGNED

  static inline Hash
  ReadHash(FILE* file){
    uint8_t data[Hash::GetSize()];
    if(!ReadBytes(file, data, Hash::GetSize()))
      return Hash();
    return Hash(data);
  }

  static inline Version
  ReadVersion(FILE* file){
    return Version(ReadShort(file), ReadShort(file), ReadShort(file));
  }

  static inline Timestamp
  ReadTimestamp(FILE* file){
    return FromUnixTimestamp(ReadUnsignedLong(file));
  }

  bool WriteBytes(FILE* file, const uint8_t* data, const int64_t& nbytes);

#define DECLARE_WRITE_SIGNED(Name, Type) \
  bool Write##Name(FILE* file, const Type##_t& val);
#define DECLARE_WRITE_UNSIGNED(Name, Type) \
  bool Write##Unsigned##Name(FILE* file, const u##Type##_t& val);
#define DECLARE_WRITE(Name, Type) \
  DECLARE_WRITE_SIGNED(Name, Type)\
  DECLARE_WRITE_UNSIGNED(Name, Type)

  FOR_EACH_NATIVE_TYPE(DECLARE_WRITE)
#undef DECLARE_WRITE
#undef DECLARE_WRITE_SIGNED
#undef DECLARE_WRITE_UNSIGNED

  static inline bool
  WriteVersion(FILE* file, const Version& version){
    return WriteShort(file, version.major())
        && WriteShort(file, version.minor())
        && WriteShort(file, version.revision());
  }

  static inline bool
  WriteTimestamp(FILE* file, const Timestamp& timestamp){
    return WriteUnsignedLong(file, ToUnixTimestamp(timestamp));
  }
}

#endif//TOKEN_UTILS_FILESYSTEM_H