#include "block.h"

namespace Token{
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

    Block* Block::Load(ByteBuffer* bb){
        Messages::Block* raw = new Messages::Block();
        raw->ParseFromArray((char*)bb->GetBytes(), bb->WrittenBytes());
        return new Block(raw);
    }

    void Block::Write(const std::string& filename){
        std::fstream fd(filename, std::ios::binary|std::ios::out|std::ios::trunc);
        GetRaw()->SerializeToOstream(&fd);
        fd.flush();
        fd.close();
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