#include "object.h"
#include "byte_buffer.h"
#include "bitfield.h"

namespace Token{
    bool BinaryObject::WriteToFile(std::fstream &file) const{
        ByteBuffer bytes;
        if(!Write(&bytes)){
            LOG(WARNING) << "couldn't encode object to bytes";
            return false;
        }

        LOG(INFO) << "writing " << bytes.GetWrittenBytes() << " bytes to file";

        file.write((char*)bytes.data(), bytes.GetWrittenBytes());
        file.flush();
        return true;
    }

    Hash BinaryObject::GetHash() const{
        CryptoPP::SHA256 func;

        ByteBuffer bytes;
        if(!Write(&bytes)){
            LOG(WARNING) << "couldn't encode object to bytes";
            return Hash();
        }

        CryptoPP::SecByteBlock hash(CryptoPP::SHA256::DIGESTSIZE);
        CryptoPP::ArraySource source(bytes.data(), bytes.GetWrittenBytes(), true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(hash.data(), hash.size())));
        return Hash::FromBytes(hash.data());
    }
}