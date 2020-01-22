#include <glog/logging.h>
#include "blockchain.h"
#include "block.h"

namespace Token{
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

    std::string Block::GetHash(){
        CryptoPP::SHA256 func;
        std::string digest;
        size_t size = GetRaw()->ByteSizeLong();
        uint8_t bytes[size];
        GetRaw()->SerializeToArray(bytes, size);
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    std::string Block::ToString(){
        return "Block(" + GetHash() + ")";
    }

    std::string Block::GetMerkleRoot(){
        std::vector<MerkleNodeItem*> items;
        for(size_t i = 0; i < GetNumberOfTransactions(); i++){
            items.push_back(GetTransactionAt(0));
        }
        MerkleTree mktree(items);
        return mktree.GetMerkleRoot();
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

    Transaction* Block::GetTransactionAt(size_t idx){
        if(idx < 0 || idx > GetNumberOfTransactions()) return nullptr;
        Transaction* tx = new Transaction();
        tx->GetRaw()->CopyFrom(GetRaw()->transactions(idx));
        return tx;
    }
}