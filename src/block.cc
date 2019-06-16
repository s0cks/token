#include "block.h"

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
        ByteBuffer bb;
        Encode(&bb);
        CryptoPP::ArraySource source(bb.GetBytes(), bb.Size(), true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    HashArray Transaction::GetHashArray(){
        HashArray result;
        CryptoPP::SHA256 func;
        ByteBuffer bb;
        Encode(&bb);
        CryptoPP::ArraySource source(bb.GetBytes(), bb.Size(), true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(result.data(), DIGEST_SIZE)));
        return result;
    }

    void Block::Encode(Token::ByteBuffer* bb){
        uint32_t size = static_cast<uint32_t>(GetRaw()->ByteSizeLong());
        bb->Resize(size);
        GetRaw()->SerializeToArray(bb->GetBytes(), size);
    }

    Block* Block::Load(const std::string& filename){
        std::fstream fd(filename, std::ios::binary|std::ios::in);
        Messages::Block* raw = new Messages::Block();
        raw->ParseFromIstream(&fd);
        return new Block(raw);
    }

    void Block::Write(const std::string& filename){
        std::fstream fd(filename, std::ios::binary|std::ios::out|std::ios::trunc);
        Token::ByteBuffer bb;
        Encode(&bb);
        fd.write(reinterpret_cast<char*>(bb.GetBytes()), bb.WrittenBytes());
        fd.flush();
    }

    std::string Block::GetHash(){
        CryptoPP::SHA256 func;
        std::string digest;
        ByteBuffer bb;
        Encode(&bb);
        CryptoPP::ArraySource source(bb.GetBytes(), bb.Size(), true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    std::string Block::GetMerkleRoot(){
        std::vector<MerkleNodeItem*> items;
        for(size_t i = 0; i < GetNumberOfTransactions(); i++) items.push_back(GetTransactionAt(i));
        MerkleTree mktree(items);
        return mktree.GetMerkleRoot();
    }
}