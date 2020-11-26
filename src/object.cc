#include "object.h"
#include "buffer.h"
#include "bitfield.h"

namespace Token{
    bool BinaryObject::WriteToFile(std::fstream& fd) const{
        intptr_t size = GetBufferSize();
        Handle<Buffer> buff = Buffer::NewInstance(size);
        if(!Write(buff)){
            LOG(WARNING) << "couldn't encode object to bytes";
            return false;
        }

        buff->WriteBytesTo(fd, size);
        return true;
    }

    Hash BinaryObject::GetHash() const{
        intptr_t size = GetBufferSize();

        CryptoPP::SHA256 func;
        Handle<Buffer> buff = Buffer::NewInstance(size);
        if(!Write(buff)){
            LOG(WARNING) << "couldn't encode object to bytes";
            return Hash();
        }

        CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
        CryptoPP::ArraySource source((uint8_t*)buff->data(), size, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
        return Hash::FromBytes(hash.data());
    }
}