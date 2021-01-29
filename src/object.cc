#include "object.h"
#include "utils/buffer.h"
#include "utils/bitfield.h"
#include "utils/file_writer.h"

namespace Token{
  bool SerializableObject::WriteToFile(BinaryFileWriter* writer) const{
    BufferPtr data = Buffer::NewInstance(GetBufferSize());
    if(!Write(data)){
      LOG(WARNING) << "cannot serialize " << ToString() << " to buffer.";
      return false;
    }
    return writer->WriteBytes(data);
  }

  Hash BinaryObject::GetHash() const{
    CryptoPP::SHA256 func;

    int64_t size = GetBufferSize();
    BufferPtr buff = Buffer::NewInstance(size);
    if(!Write(buff)){
      LOG(WARNING) << "couldn't encode object to bytes";
      return Hash();
    }

    CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
    CryptoPP::ArraySource source((uint8_t*) buff->data(),
                                 size,
                                 true,
                                 new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
    return Hash(hash.data(), CryptoPP::SHA256::DIGESTSIZE);
  }

  const int32_t ObjectTag::kMagic = 0xFAFE;
}