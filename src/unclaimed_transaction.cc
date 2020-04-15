#include <dirent.h>
#include <glog/logging.h>
#include "common.h"
#include "unclaimed_transaction.h"
#include "block_chain.h"

namespace Token{
    bool UnclaimedTransaction::GetBytes(CryptoPP::SecByteBlock& bytes) const{
        Proto::BlockChain::UnclaimedTransaction raw;
        raw << (*this);
        bytes.resize(raw.ByteSizeLong());
        return raw.SerializeToArray(bytes.data(), bytes.size());
    }

    static pthread_rwlock_t kPoolLock = PTHREAD_RWLOCK_INITIALIZER;
#define READ_LOCK pthread_rwlock_tryrdlock(&kPoolLock);
#define WRITE_LOCK pthread_rwlock_trywrlock(&kPoolLock);
#define UNLOCK pthread_rwlock_unlock(&kPoolLock);

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
        READ_LOCK;
        UnclaimedTransactionPool* pool = GetInstance();
        std::string filename = pool->GetPath(hash);
        if(!FileExists(filename)){
            UNLOCK;
            return nullptr;
        }
        UnclaimedTransaction* utxo = new UnclaimedTransaction();
        if(!LoadUnclaimedTransaction(filename, utxo)){
            delete utxo;
            UNLOCK;
            return nullptr;
        }
        UNLOCK;
        return utxo;
    }

    UnclaimedTransaction* UnclaimedTransactionPool::RemoveUnclaimedTransaction(const uint256_t& hash){
        WRITE_LOCK;
        UnclaimedTransactionPool* pool = GetInstance();
        std::string filename = pool->GetPath(hash);
        if(!FileExists(filename)){
            UNLOCK;
            return nullptr;
        }
        UnclaimedTransaction* utxo = new UnclaimedTransaction();
        if(!LoadUnclaimedTransaction(filename, utxo) || !DeleteFile(filename) || !pool->UnregisterPath(hash)){
            delete utxo;
            UNLOCK;
            return nullptr;
        }
        UNLOCK;
        LOG(INFO) << "removed utxo: " << hash;
        return utxo;
    }

    bool UnclaimedTransactionPool::PutUnclaimedTransaction(UnclaimedTransaction* utxo){
        WRITE_LOCK;
        uint256_t hash = utxo->GetHash();
        UnclaimedTransactionPool* pool = GetInstance();
        std::string filename = pool->GetPath(hash);
        if(FileExists(filename)){
            UNLOCK;
            return false;
        }
        if(!SaveUnclaimedTransaction(filename, utxo)){
            UNLOCK;
            return false;
        }
        if(!pool->RegisterPath(hash, filename)){
            UNLOCK;
            return false;
        }
        UNLOCK;
        LOG(INFO) << "added utxo: " << hash;
        return true;
    }

    bool UnclaimedTransactionPool::HasUnclaimedTransaction(const uint256_t& hash){
        READ_LOCK;
        UnclaimedTransactionPool* pool = GetInstance();
        std::string filename = pool->GetPath(hash);
        bool found = FileExists(filename);
        UNLOCK;
        return found;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::vector<uint256_t>& utxos){
        READ_LOCK;
        UnclaimedTransactionPool* pool = GetInstance();
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(pool->GetRoot().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (pool->GetRoot() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                UnclaimedTransaction utxo;
                if(!pool->LoadUnclaimedTransaction(filename, &utxo)){
                    LOG(WARNING) << "couldn't load unclaimed transaction data from: " << filename;
                    utxos.clear();
                    UNLOCK;
                    return false;
                }
                utxos.push_back(utxo.GetHash());
            }
            closedir(dir);
            UNLOCK;
            LOG(INFO) << "found " << utxos.size() << " utxos for all users";
            return true;
        }
        UNLOCK;
        return false;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::string& user, std::vector<uint256_t>& utxos){
        READ_LOCK;
        UnclaimedTransactionPool* pool = GetInstance();
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(pool->GetRoot().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string filename(ent->d_name);
                if(!EndsWith(filename, ".dat")) continue;
                UnclaimedTransaction utxo;
                if(!pool->LoadUnclaimedTransaction(filename, &utxo)){
                    LOG(WARNING) << "couldn't load unclaimed transaction data from: " << filename;
                    utxos.clear();
                    UNLOCK;
                    return false;
                }
                if(utxo.GetUser() != user) continue;
                utxos.push_back(utxo.GetHash());
            }
            closedir(dir);
            UNLOCK;
            LOG(INFO) << "found " << utxos.size() << " utxos for " << user;
            return true;
        }
        UNLOCK;
        return false;
    }
}