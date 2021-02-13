#include "utils/file_reader.h"

namespace token{
  bool FileReader::ReadBytes(uint8_t* data, const int64_t& size) const{
    if(fread(data, sizeof(uint8_t), size, file_) != (sizeof(uint8_t)*size)){
      LOG(WARNING) << "cannot read " << size << " bytes from " << GetFilename() << ": " << strerror(errno);
      return false;
    }
    return true;
  }

#define DEFINE_READ_SIGNED(Name, Type) \
  Type FileReader::Read##Name() const{ \
    Type result = 0;                   \
    if(fread(&result, sizeof(Type), 1, file_) != 1){ \
      LOG(WARNING) << "cannot read " << #Name << " from " << GetFilename() << ": " << strerror(errno); \
      return 0;                        \
    }                                  \
    return result;                     \
  }
#define DEFINE_READ_UNSIGNED(Name, Type) \
  u##Type FileReader::ReadUnsigned##Name() const{ \
    u##Type result = 0;                  \
    if(fread(&result, sizeof(u##Type), 1, file_) != 1){ \
      LOG(WARNING) << "cannot read Unsigned" << #Name << " from " << GetFilename() << ": " << strerror(errno); \
      return 0;                          \
    }                                    \
    return result;                       \
  }
#define DEFINE_READ(Name, Type) \
  DEFINE_READ_SIGNED(Name, Type)\
  DEFINE_READ_UNSIGNED(Name, Type)

  FOR_EACH_RAW_TYPE(DEFINE_READ)
#undef DEFINE_READ
#undef DEFINE_READ_SIGNED
#undef DEFINE_READ_UNSIGNED
}