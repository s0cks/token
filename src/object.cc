#include "object.h"
#include "utils/buffer.h"
#include "bitfield.h"

namespace Token{
    bool BinaryObject::ToSlice(leveldb::Slice* slice) const{
        if(!slice)
            return false;
        Hash hash = GetHash();
        Buffer buffer(GetBufferSize() + Hash::GetSize());
        buffer.PutHash(hash);
        if(!Write(&buffer))
            return false;
        (*slice) = leveldb::Slice(buffer.data(), buffer.GetWrittenBytes());
        return true;
    }

    Hash BinaryObject::GetHash() const{
        intptr_t size = GetBufferSize();

        CryptoPP::SHA256 func;
        Buffer buff(size);
        if(!Write(&buff)){
            LOG(WARNING) << "couldn't encode object to bytes";
            return Hash();
        }

        CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
        CryptoPP::ArraySource source((uint8_t*)buff.data(), size, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
        return Hash(hash.data(), CryptoPP::SHA256::DIGESTSIZE);
    }
}