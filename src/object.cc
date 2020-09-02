#include "object.h"
#include "bytes.h"
#include "bitfield.h"

namespace Token{
    enum ObjectHeaderLayout{
        kTypeFieldPosition = 0,
        kTypeFieldSize = 16,
        kSizeFieldPosition = kTypeFieldPosition+kTypeFieldSize,
        kSizeFieldSize = 32
    };

    class ObjectHeaderTypeField : public BitField<ObjectHeader, Type, kTypeFieldPosition, kTypeFieldSize>{};
    class ObjectHeaderSizeField : public BitField<ObjectHeader, uint32_t, kSizeFieldPosition, kSizeFieldSize>{};

    ObjectHeader CreateObjectHeader(Object* obj){
        ObjectHeader header = 0;
        header = ObjectHeaderTypeField::Update(obj->GetType(), header);
        header = ObjectHeaderSizeField::Update(obj->GetBufferSize(), header);
        return header;
    }

    Type GetObjectHeaderType(ObjectHeader header){
        return ObjectHeaderTypeField::Decode(header);
    }

    uint32_t GetObjectHeaderSize(ObjectHeader header){
        return ObjectHeaderSizeField::Decode(header);
    }

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