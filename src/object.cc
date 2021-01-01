#include "object.h"
#include "utils/buffer.h"
#include "bitfield.h"

namespace Token{
    Hash BinaryObject::GetHash() const{
        CryptoPP::SHA256 func;

        int64_t size = GetBufferSize();
        BufferPtr buff = Buffer::NewInstance(size);
        if(!Write(buff)){
            LOG(WARNING) << "couldn't encode object to bytes";
            return Hash();
        }

        CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
        CryptoPP::ArraySource source((uint8_t*)buff->data(), size, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
        return Hash(hash.data(), CryptoPP::SHA256::DIGESTSIZE);
    }
}