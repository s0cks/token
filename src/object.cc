#include "object.h"
#include "bytes.h"

namespace Token{
    bool Object::WriteToFile(std::fstream &file) const{
        size_t size = GetBufferSize();
        ByteBuffer bytes(size);
        if(!Encode(&bytes)){
            LOG(WARNING) << "couldn't encode object to bytes";
            return false;
        }
        file.write((char*)bytes.data(), bytes.GetWrittenBytes());
        file.flush();
        return true;
    }

    uint256_t Object::GetHash() const{
        size_t size = GetBufferSize();
        CryptoPP::SHA256 func;

        ByteBuffer bytes(size);
        if(!Encode(&bytes)){
            LOG(WARNING) << "couldn't encode object to bytes";
            return uint256_t();
        }

        CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
        CryptoPP::ArraySource source(bytes.data(), bytes.GetWrittenBytes(), true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
        return uint256_t(hash.data());
    }
}