#include "json.h"
#include "object.h"
#include "utils/buffer.h"
#include "utils/bitfield.h"

namespace token{
  BufferPtr SerializableObject::ToBuffer() const{
    BufferPtr buffer = Buffer::NewInstance(GetBufferSize());
    if(!Write(buffer)){
      LOG(WARNING) << "cannot serialize object " << ToString() << " to buffer of size: " << GetBufferSize();
      return BufferPtr(nullptr);
    }
    return buffer;
  }

  BufferPtr SerializableObject::ToBufferTagged() const{
    BufferPtr buffer = Buffer::NewInstance(sizeof(RawObjectTag)+GetBufferSize());
    if(!buffer->PutObjectTag(GetTag())){
      LOG(WARNING) << "cannot serialize object tag for " << ToString() << " to buffer of size: " << GetBufferSize();
      return BufferPtr(nullptr);
    }

    if(!Write(buffer)){
      LOG(WARNING) << "cannot serialize object " << ToString() << " to buffer of size: " << GetBufferSize();
      return BufferPtr(nullptr);
    }
    return buffer;
  }

  bool SerializableObject::ToFile(const std::string& filename) const{
    std::fstream fd(filename, std::ios::out|std::ios::binary);
    BufferPtr buffer = ToBufferTagged();
    return buffer && buffer->WriteTo(fd);
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

  bool TransactionReference::Write(Json::Writer& writer) const{
    return writer.StartObject()
        && Json::SetField(writer, "hash", transaction_)
        && Json::SetField(writer, "index", index_)
        && writer.EndObject();
  }
}