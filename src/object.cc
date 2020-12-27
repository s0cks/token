#include "object.h"
#include "utils/buffer.h"
#include "bitfield.h"

namespace Token{
    Hash BinaryObject::GetHash() const{
        intptr_t size = GetBufferSize();

        CryptoPP::SHA256 func;
        Buffer buff(size);
        if(!Encode(&buff)){
            LOG(WARNING) << "couldn't encode object to bytes";
            return Hash();
        }

        CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
        CryptoPP::ArraySource source((uint8_t*)buff.data(), size, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
        return Hash(hash.data(), CryptoPP::SHA256::DIGESTSIZE);
    }
}