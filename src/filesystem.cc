#include "filesystem.h"

namespace token{
  bool ReadBytes(FILE* file, uint8_t* data, const int64_t& size){
    if(fread(data, sizeof(uint8_t), size, file) != (sizeof(uint8_t)*size)){
      LOG(WARNING) << "cannot read " << size << " bytes from file: " << strerror(errno);
      return false;
    }
    return true;
  }

#define DEFINE_READ_SIGNED(Name, Type) \
  Type Read##Name(FILE* file){         \
    Type result = 0;                   \
    if(fread(&result, sizeof(Type), 1, file) != 1){ \
      LOG(WARNING) << "cannot read " << #Name << " from file: " << strerror(errno); \
      return 0;                        \
    }                                  \
    return result;                     \
  }
#define DEFINE_READ_UNSIGNED(Name, Type) \
  u##Type ReadUnsigned##Name(FILE* file){\
    u##Type result = 0;                  \
    if(fread(&result, sizeof(u##Type), 1, file) != 1){ \
      LOG(WARNING) << "cannot read Unsigned" << #Name << " from file: " << strerror(errno); \
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

  bool WriteBytes(FILE* file, const uint8_t* data, const int64_t& size){
    if((fwrite(data, sizeof(uint8_t), size, file)) != (sizeof(uint8_t)*size)){
      LOG(WARNING) << "cannot write " << size << " bytes to file: " << strerror(errno);
      return false;
    }
    return true;
  }

#define DEFINE_WRITE_SIGNED(Name, Type) \
  bool Write##Name(FILE* file, const Type& val){ \
    if((fwrite(&val, sizeof(Type), 1, file)) != 1){ \
      LOG(WARNING) << "cannot write " << #Name << " to file: " << strerror(errno); \
      return false;                     \
    }                                   \
    return true;                        \
  }
#define DEFINE_WRITE_UNSIGNED(Name, Type) \
  bool WriteUnsigned##Name(FILE* file, const u##Type& val){ \
    if((fwrite(&val, sizeof(u##Type), 1, file)) != 1){      \
      LOG(WARNING) << "cannot write Unsigned" << #Name << " to file: " << strerror(errno); \
      return false;                       \
    }                                     \
    return true;                          \
  }

#define DEFINE_WRITE(Name, Type) \
  DEFINE_WRITE_SIGNED(Name, Type)\
  DEFINE_WRITE_UNSIGNED(Name, Type)
  FOR_EACH_RAW_TYPE(DEFINE_WRITE)
#undef DEFINE_WRITE
#undef DEFINE_WRITE_SIGNED
#undef DEFINE_WRITE_UNSIGNED
}