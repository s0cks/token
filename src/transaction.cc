#include "transaction.h"

namespace Token{
    void Transaction::Accept(Token::TransactionVisitor* vis) const{
        int i;
        for(i = 0; i < GetNumberOfInputs(); i++){
            vis->VisitInput(GetInputAt(i));
        }
        for(i = 0; i < GetNumberOfOutputs(); i++){
            vis->VisitOutput(GetOutputAt(i));
        }
    }

    void Transaction::Encode(Token::ByteBuffer *bb){
        uint32_t size = static_cast<uint32_t>(GetRaw()->ByteSizeLong());
        bb->Resize(size);
        GetRaw()->SerializeToArray(bb->GetBytes(), size);
    }

    std::string Transaction::GetHash(){
        CryptoPP::SHA256 func;
        std::string digest;
        size_t size = GetRaw()->ByteSizeLong();
        uint8_t bytes[size];
        GetRaw()->SerializeToArray(bytes, size);
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    HashArray Transaction::GetHashArray(){
        HashArray result;
        CryptoPP::SHA256 func;
        size_t size = GetRaw()->ByteSizeLong();
        uint8_t bytes[size];
        GetRaw()->SerializeToArray(bytes, size);
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(result.data(), DIGEST_SIZE)));
        return result;
    }

    void Input::Encode(ByteBuffer* bb) const{
        uint32_t size = static_cast<uint32_t>(GetRaw()->ByteSizeLong());
        bb->Resize(size);
        GetRaw()->SerializeToArray(bb->GetBytes(), size);
    }

    std::string Input::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        ByteBuffer bb;
        Encode(&bb);
        CryptoPP::ArraySource source(bb.GetBytes(), bb.Size(), true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    void Output::Encode(Token::ByteBuffer *bb) const{
        uint32_t size = static_cast<uint32_t>(GetRaw()->ByteSizeLong());
        bb->Resize(size);
        GetRaw()->SerializeToArray(bb->GetBytes(), size);
    }

    std::string Output::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        ByteBuffer bb;
        Encode(&bb);
        CryptoPP::ArraySource source(bb.GetBytes(), bb.Size(), true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }
}