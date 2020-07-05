#include "unclaimed_transaction.h"
#include "block_chain.h"
#include "crash_report.h"

namespace Token{
//######################################################################################################################
//                                          Unclaimed Transaction
//######################################################################################################################
    UnclaimedTransaction* UnclaimedTransaction::NewInstance(const uint256_t &hash, uint32_t index, const std::string& user){
        UnclaimedTransaction* instance = (UnclaimedTransaction*)Allocator::Allocate(sizeof(UnclaimedTransaction));
        new (instance)UnclaimedTransaction(hash, index, user);
        return instance;
    }

    UnclaimedTransaction* UnclaimedTransaction::NewInstance(const UnclaimedTransaction::RawType& raw){
        return NewInstance(HashFromHexString(raw.tx_hash()), raw.tx_index(), raw.user());
    }

    UnclaimedTransaction* UnclaimedTransaction::NewInstance(std::fstream& fd){
        UnclaimedTransaction::RawType raw;
        if(!raw.ParseFromIstream(&fd)) return nullptr;
        return NewInstance(raw);
    }

    std::string UnclaimedTransaction::ToString() const{
        std::stringstream stream;
        stream << "UnclaimedTransaction(" << hash_ << "[" << index_ << "])";
        return stream.str();
    }

    bool UnclaimedTransaction::Encode(Token::UnclaimedTransaction::RawType &raw) const{
        raw.set_tx_hash(HexString(hash_));
        raw.set_tx_index(index_);
        raw.set_user(user_);
        return true;
    }

//######################################################################################################################
//                                          Unclaimed Transaction Pool
//######################################################################################################################
    static std::recursive_mutex mutex_;
    static leveldb::DB* index_ = nullptr;
    static UnclaimedTransactionPool::State state_ = UnclaimedTransactionPool::kUninitialized;

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/utxos";
    }

    static inline leveldb::DB*
    GetIndex(){
        return index_;
    }

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    void UnclaimedTransactionPool::SetState(UnclaimedTransactionPool::State state){
        LOCK_GUARD;
        state_ = state;
    }

    UnclaimedTransactionPool::State UnclaimedTransactionPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void UnclaimedTransactionPool::Initialize(){
        if(!IsUninitialized()){
            std::stringstream ss;
            ss << "Cannot re-initialize unclaimed transaction pool";
            CrashReport::GenerateAndExit(ss);
        }

        SetState(kInitializing);
        std::string filename = GetDataDirectory();
        if(!FileExists(filename)){
            if(!CreateDirectory(filename)){
                std::stringstream ss;
                ss << "Cannot create UnclaimedTransactionPool data directory: " << filename;
                CrashReport::GenerateAndExit(ss);
            }
        }

        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, (filename + "/index"), &index_).ok()){
            std::stringstream ss;
            ss << "Couldn't open unclaimed transaction pool index: " << (filename + "/index");
            CrashReport::GenerateAndExit(ss);
        }

        SetState(kInitialized);
#ifdef TOKEN_DEBUG
        LOG(INFO) << "initialized the unclaimed transaction pool";
#endif//TOKEN_DEBUG
    }

    bool UnclaimedTransactionPool::HasUnclaimedTransaction(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = HexString(hash);
        std::string filename;
        LOCK_GUARD;
        return GetIndex()->Get(options, key, &filename).ok();
    }

    static inline std::string
    GetNewUnclaimedTransactionFilename(const uint256_t& hash){
        std::string hashString = HexString(hash);
        std::string front = hashString.substr(0, 8);
        std::string tail = hashString.substr(hashString.length() - 8, hashString.length());
        std::string filename = GetDataDirectory() + "/" + front + ".dat";
        if(FileExists(filename)){
            filename = GetDataDirectory() + "/" + tail + ".dat";
        }
        return filename;
    }

    static inline std::string
    GetUnclaimedTransactionFilename(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = HexString(hash);
        std::string filename;
        if(!GetIndex()->Get(options, key, &filename).ok()) return GetNewUnclaimedTransactionFilename(hash);
        return filename;
    }

    UnclaimedTransaction* UnclaimedTransactionPool::GetUnclaimedTransaction(const uint256_t& hash){
        if(!HasUnclaimedTransaction(hash)) return nullptr;
        LOCK_GUARD;
        std::string filename = GetUnclaimedTransactionFilename(hash);
        return UnclaimedTransaction::NewInstance(filename);
    }

    void UnclaimedTransactionPool::RemoveUnclaimedTransaction(const uint256_t& hash){
        if(!HasUnclaimedTransaction(hash)) return;
        LOCK_GUARD;
        std::string filename = GetUnclaimedTransactionFilename(hash);
        if(!DeleteFile(filename)){
            std::stringstream ss;
            ss << "Couldn't remove unclaimed transaction " << hash << " file: " << filename;
            CrashReport::GenerateAndExit(ss);
        }

        leveldb::WriteOptions options;
        if(!GetIndex()->Delete(options, HexString(hash)).ok()){
            std::stringstream ss;
            ss << "Couldn't remove unclaimed transaction " << hash << " from index";
            CrashReport::GenerateAndExit(ss);
        }
#ifdef TOKEN_DEBUG
        LOG(INFO) << "removed unclaimed transaction: " << hash;
#endif//TOKEN_DEBUG
    }

    void UnclaimedTransactionPool::PutUnclaimedTransaction(UnclaimedTransaction* utxo){
        uint256_t hash = utxo->GetHash();
        if(HasUnclaimedTransaction(hash)){
            std::stringstream ss;
            ss << "Couldn't add duplicate unclaimed transaction: " << hash;
            CrashReport::GenerateAndExit(ss);
        }

        std::string filename = GetNewUnclaimedTransactionFilename(hash);
        std::fstream fd(filename, std::ios::out|std::ios::binary);
        if(!utxo->WriteToFile(fd)){
            std::stringstream ss;
            ss << "Couldn't write unclaimed transaction " << hash << " to file: " << filename;
            CrashReport::GenerateAndExit(ss);
        }

        LOCK_GUARD;
        leveldb::WriteOptions options;
        std::string key = HexString(hash);
        if(!GetIndex()->Put(options, key, filename).ok()){
            std::stringstream ss;
            ss << "Couldn't index unclaimed transaction: " << hash;
            CrashReport::GenerateAndExit(ss);
        }
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(std::vector<uint256_t>& utxos){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                UnclaimedTransaction* utxo = UnclaimedTransaction::NewInstance(filename);
                utxos.push_back(utxo->GetHash());
            }
            closedir(dir);
            return true;
        }

        return false;
    }

    bool UnclaimedTransactionPool::GetUnclaimedTransactions(const std::string& user, std::vector<uint256_t>& utxos){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                UnclaimedTransaction* utxo = UnclaimedTransaction::NewInstance(filename);
                // if(utxo.GetUser() != user) continue;
                utxos.push_back(utxo->GetHash());
            }
            closedir(dir);
            return true;
        }
        return false;
    }

    bool UnclaimedTransactionPool::Accept(UnclaimedTransactionPoolVisitor* vis){
        if(!vis->VisitStart()) return false;

        std::vector<uint256_t> utxos;
        if(!GetUnclaimedTransactions(utxos)) return false;

        for(auto& it : utxos){
            UnclaimedTransaction* utxo;
            if(!(utxo = GetUnclaimedTransaction(it))) return false;
            if(!vis->Visit(utxo)) return false;
        }

        return vis->VisitEnd();
    }

    UnclaimedTransaction* UnclaimedTransactionPool::GetUnclaimedTransaction(const uint256_t &tx_hash, uint32_t tx_index){
        std::vector<uint256_t> utxos;
        if(!GetUnclaimedTransactions(utxos)) {
#ifdef TOKEN_DEBUG
            LOG(WARNING) << "couldn't get unclaimed transactions";
#endif//TOKEN_DEBUG
            return nullptr;
        }
        for(auto& it : utxos){
#ifdef TOKEN_DEBUG
            LOG(WARNING) << "checking utxo: " << it;
#endif//TOKEN_DEBUG

            UnclaimedTransaction* utxo;
            if(!(utxo = GetUnclaimedTransaction(it))){
#ifdef TOKEN_DEBUG
                LOG(WARNING) << "couldn't get unclaimed transaction: " << it;
#endif//TOKEN_DEBUG
                return nullptr;
            }

#ifdef TOKEN_DEBUG
            LOG(WARNING) << "unclaimed transaction: " << it << " := " << utxo->GetTransaction() << "[" << utxo->GetIndex() << "]";
#endif//TOKEN_DEBUG
            if(utxo->GetTransaction() == tx_hash && utxo->GetIndex() == tx_index) return utxo;
        }
#ifdef TOKEN_DEBUG
        LOG(WARNING) << "couldn't find unclaimed transaction: " << tx_hash << "[" << tx_index << "]";
#endif//TOKEN_DEBUG
        return nullptr;
    }
}
