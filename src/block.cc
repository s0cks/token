#include <glog/logging.h>
#include <allocator.h>
#include "block.h"

namespace Token{
    void Block::Encode(Token::ByteBuffer* bb){
        uint32_t size = static_cast<uint32_t>(GetRaw()->ByteSizeLong());
        bb->Resize(size);
        GetRaw()->SerializeToArray(bb->GetBytes(), size);
    }

    bool Block::Equals(const Token::Messages::BlockHeader &head){
        if(GetHeight() != head.height()){
            LOG(WARNING) << "block heights don't match";
            return false;
        }
        if(GetHash() != head.hash()){
            LOG(WARNING) << "block hashes don't match";
            return false;
        }
        if(GetPreviousHash() != head.previous_hash()){
            LOG(WARNING) << "block previous hashes don't match";
            return false;
        }
        if(GetMerkleRoot() != head.merkle_root()){
            LOG(WARNING) << "block merkle roots don't match";
            return false;
        }
        return true;
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
        size_t size = GetRaw()->ByteSizeLong();
        uint8_t bytes[size];
        GetRaw()->SerializeToArray(bytes, size);
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    std::string Block::GetMerkleRoot(){
        std::vector<MerkleNodeItem*> items;
        for(size_t i = 0; i < GetNumberOfTransactions(); i++) items.push_back(GetTransactionAt(i));
        MerkleTree mktree(items);
        return mktree.GetMerkleRoot();
    }

    Token::Messages::Block* Block::GetAsMessage(){
        Token::Messages::Block* msg = new Token::Messages::Block();
        msg->CopyFrom(raw_);
        return msg;
    }

    void* Block::operator new(size_t size){
        return reinterpret_cast<Block*>(Allocator::Allocate(size));
    }

    Block::Block():
        raw_(){
        GetRaw()->set_height(0);
        GetRaw()->set_previous_hash(GetGenesisPreviousHash());
    }

    Block::Block(Token::Block* parent):
        raw_(){
        GetRaw()->set_height(parent->GetHeight() + 1);
        GetRaw()->set_previous_hash(parent->GetHash());
    }

    Block::Block(uint32_t height, const std::string &previous_hash):
        raw_(){
        GetRaw()->set_previous_hash(previous_hash);
        GetRaw()->set_height(height);
    }

    Block::~Block(){}

    bool Block::LoadBlockFromFile(const std::string& filename){
        std::fstream fd(filename, std::ios::binary|std::ios::in);
        return GetRaw()->ParseFromIstream(&fd);
    }

    Block* Block::Decode(Messages::Block* msg){
        return new Block(msg);
    }
}