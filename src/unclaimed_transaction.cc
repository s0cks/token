#include <glog/logging.h>
#include "common.h"
#include "unclaimed_transaction.h"

namespace Token{
    bool UnclaimedTransaction::GetBytes(CryptoPP::SecByteBlock& bytes) const{
        Proto::BlockChain::UnclaimedTransaction raw;
        raw << (*this);
        bytes.resize(raw.ByteSizeLong());
        return raw.SerializeToArray(bytes.data(), bytes.size());
    }

    UnclaimedTransactionPool* UnclaimedTransactionPool::GetInstance(){
        static UnclaimedTransactionPool instance;
        return &instance;
    }

    bool UnclaimedTransactionPool::Initialize(){
        UnclaimedTransactionPool* pool = GetInstance();
        if(!FileExists(pool->GetRoot())){
            if(!CreateDirectory(pool->GetRoot())) return false;
        }
        return pool->InitializeIndex();
    }

    bool UnclaimedTransactionPool::LoadUnclaimedTransaction(const std::string &filename, UnclaimedTransaction *utxo){
        if(!FileExists(filename)) return false;
        std::fstream fd(filename, std::ios::in|std::ios::binary);
        Proto::BlockChain::UnclaimedTransaction raw;
        if(!raw.ParseFromIstream(&fd)) return false;
        (*utxo) = UnclaimedTransaction(raw);
        return true;
    }

    bool UnclaimedTransactionPool::SaveUnclaimedTransaction(const std::string &filename, Token::UnclaimedTransaction *utxo){
        if(FileExists(filename)) return false;
        std::fstream fd(filename, std::ios::out|std::ios::binary);
        Proto::BlockChain::UnclaimedTransaction raw;
        raw << (*utxo);
        return raw.SerializeToOstream(&fd);
    }

    UnclaimedTransaction* UnclaimedTransactionPool::GetUnclaimedTransaction(const uint256_t& hash){
        UnclaimedTransactionPool* pool = GetInstance();
        std::string filename = pool->GetPath(hash);
        if(!FileExists(filename)) return nullptr;
        UnclaimedTransaction* utxo = new UnclaimedTransaction();
        if(!LoadUnclaimedTransaction(filename, utxo)){
            delete utxo;
            return nullptr;
        }
        return utxo;
    }

    UnclaimedTransaction* UnclaimedTransactionPool::RemoveUnclaimedTransaction(const uint256_t& hash){
        UnclaimedTransactionPool* pool = GetInstance();
        std::string filename = pool->GetPath(hash);
        if(!FileExists(filename)) return nullptr;
        UnclaimedTransaction* utxo = new UnclaimedTransaction();
        if(!LoadUnclaimedTransaction(filename, utxo) || !DeleteFile(filename) || !pool->UnregisterPath(hash)){
            delete utxo;
            return nullptr;
        }
        return utxo;
    }

    bool UnclaimedTransactionPool::PutUnclaimedTransaction(UnclaimedTransaction* utxo){
        uint256_t hash = utxo->GetHash();
        UnclaimedTransactionPool* pool = GetInstance();
        std::string filename = pool->GetPath(hash);
        if(FileExists(filename)) return false;
        if(!SaveUnclaimedTransaction(filename, utxo)) return false;
        return pool->RegisterPath(hash, filename);
    }

    bool UnclaimedTransactionPool::HasUnclaimedTransaction(const uint256_t& hash){
        UnclaimedTransactionPool* pool = GetInstance();
        std::string filename = pool->GetPath(hash);
        return FileExists(filename);
    }
}