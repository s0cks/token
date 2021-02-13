#include "utils/file_writer.h"

namespace token{
  bool FileWriter::WriteBytes(uint8_t* bytes, const int64_t& size) const{
    if((fwrite(bytes, sizeof(uint8_t), size, file_)) != (sizeof(uint8_t)*size)){
      LOG(WARNING) << "cannot write " << size << " bytes to " << GetFilename() << ": " << strerror(errno);
      return false;
    }
    return true;
  }

#define DEFINE_WRITE_SIGNED(Name, Type) \
  bool FileWriter::Write##Name(const Type& val) const{ \
    if((fwrite(&val, sizeof(Type), 1, file_)) != 1){ \
      LOG(WARNING) << "cannot write " << #Name << " to file " << GetFilename() << ": " << strerror(errno); \
      return false;                     \
    }                                   \
    return true;                        \
  }
#define DEFINE_WRITE_UNSIGNED(Name, Type) \
  bool FileWriter::WriteUnsigned##Name(const u##Type& val) const{ \
    if((fwrite(&val, sizeof(u##Type), 1, file_)) != 1){     \
      LOG(WARNING) << "cannot write Unsigned" << #Name << " to file " << GetFilename() << ": " << strerror(errno); \
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