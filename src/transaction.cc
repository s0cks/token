#include "transaction.h"
#include "keychain.h"
#include "block_chain.h"
#include "server.h"
#include "unclaimed_transaction.h"

namespace Token{
    UnclaimedTransaction* Input::GetUnclaimedTransaction() const{
        return UnclaimedTransactionPool::GetUnclaimedTransaction(GetTransactionHash(), GetOutputIndex());
    }
//######################################################################################################################
//                                          Transaction
//######################################################################################################################
    Transaction* Transaction::NewInstance(std::fstream& fd, size_t size){
        Buffer buff(size);
        buff.ReadBytesFrom(fd, size);
        return new Transaction(&buff);
    }

    bool Transaction::Accept(TransactionVisitor* vis) const{
        if(!vis->VisitStart()) return false;

        int64_t idx;
        // visit the inputs
        {
            if(!vis->VisitInputsStart()) return false;
            for(idx = 0;
                idx < GetNumberOfInputs();
                idx++){
                if(!vis->VisitInput(inputs_[idx]))
                    return false;
            }
            if(!vis->VisitInputsEnd()) return false;
        }
        // visit the outputs
        {
            if(!vis->VisitOutputsStart()) return false;
            for(idx = 0;
                idx < GetNumberOfOutputs();
                idx++){
                if(!vis->VisitOutput(outputs_[idx]))
                    return false;
            }
            if(!vis->VisitOutputsEnd()) return false;
        }
        return vis->VisitEnd();
    }

    bool Transaction::Sign(){
        CryptoPP::SecByteBlock bytes;
        /*
          TODO:
            if(!Encode(bytes)){
                LOG(WARNING) << "couldn't serialize transaction to bytes";
                return false;
            }
         */

        CryptoPP::RSA::PrivateKey privateKey;
        CryptoPP::RSA::PublicKey publicKey;
        Keychain::LoadKeys(&privateKey, &publicKey);

        try{
            LOG(INFO) << "signing transaction: " << GetHash();
            CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA256>::Signer signer(privateKey);
            CryptoPP::AutoSeededRandomPool rng;
            CryptoPP::SecByteBlock sigData(signer.MaxSignatureLength());

            size_t length = signer.SignMessage(rng, bytes.data(), bytes.size(), sigData);
            sigData.resize(length);

            std::string signature;
            CryptoPP::ArraySource source(sigData.data(), sigData.size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(signature)));

            LOG(INFO) << "signature: " << signature;
            signature_ = signature;
            return true;
        } catch(CryptoPP::Exception& ex){
            LOG(ERROR) << "error occurred signing transaction: " << ex.GetWhat();
            return false;
        }
    }

    static std::mutex mutex_;
    static std::condition_variable cond_;

    static TransactionPool::State state_ = TransactionPool::kUninitialized;
    static TransactionPool::Status status_ = TransactionPool::kOk;
    static leveldb::DB* index_ = nullptr;

#define POOL_OK_STATUS(Status, Message) \
    LOG(INFO) << (Message);             \
    SetStatus((Status));
#define POOL_OK(Message) \
    POOL_OK_STATUS(TransactionPool::kOk, (Message));
#define POOL_WARNING_STATUS(Status, Message) \
    LOG(WARNING) << (Message);               \
    SetStatus((Status));
#define POOL_WARNING(Message) \
    POOL_WARNING_STATUS(TransactionPool::kWarning, (Message))
#define POOL_ERROR_STATUS(Status, Message) \
    LOG(ERROR) << (Message);               \
    SetStatus((Status));
#define POOL_ERROR(Message) \
    POOL_ERROR_STATUS(TransactionPool::kError, (Message))
#define LOCK_GUARD std::lock_guard<std::mutex> guard(mutex_)
#define LOCK std::unique_lock<std::mutex> lock(mutex_)
#define WAIT cond_.wait(lock)
#define SIGNAL_ONE cond_.notify_one()
#define SIGNAL_ALL cond_.notify_all()

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/txs";
    }

    static inline std::string
    GetIndexFilename(){
        return GetDataDirectory() + "/index";
    }

    static inline std::string
    GetNewDataFilename(const Hash& hash){
        std::string hashString = hash.HexString();
        std::string front = hashString.substr(0, 8);
        std::string tail = hashString.substr(hashString.length() - 8, hashString.length());
        std::string filename = GetDataDirectory() + "/" + front + ".dat";
        if(FileExists(filename)){
            filename = GetDataDirectory() + "/" + tail + ".dat";
        }
        return filename;
    }

    static inline leveldb::DB*
    GetIndex(){
        return index_;
    }

    static inline std::string
    GetDataFilename(const Hash& hash){
        leveldb::ReadOptions options;
        std::string key = hash.HexString();
        std::string filename;
        return GetIndex()->Get(options, key, &filename).ok() ?
               filename :
               GetNewDataFilename(hash);
    }

    TransactionPool::State TransactionPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void TransactionPool::SetState(const TransactionPool::State& state){
        LOCK_GUARD;
        state_ = state;
    }

    TransactionPool::Status TransactionPool::GetStatus(){
        LOCK_GUARD;
        return status_;
    }

    void TransactionPool::SetStatus(const Status& status){
        LOCK_GUARD;
        status_ = status;
    }

    std::string TransactionPool::GetStatusMessage(){
        std::stringstream ss;
        LOCK_GUARD;

        ss << "[";
        switch(state_){
#define DEFINE_STATE_MESSAGE(Name) \
            case TransactionPool::k##Name: \
                ss << #Name; \
                break;
            FOR_EACH_UTXOPOOL_STATE(DEFINE_STATE_MESSAGE)
#undef DEFINE_STATE_MESSAGE
        }
        ss << "] ";

        switch(status_){
#define DEFINE_STATUS_MESSAGE(Name) \
            case TransactionPool::k##Name:{ \
                ss << #Name; \
                break; \
            }
            FOR_EACH_UTXOPOOL_STATUS(DEFINE_STATUS_MESSAGE)
#undef DEFINE_STATUS_MESSAGE
        }
        return ss.str();
    }

    bool TransactionPool::Accept(TransactionPoolVisitor* vis){
        if(!vis->VisitStart()) return false;

        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                Transaction* tx = Transaction::NewInstance(filename);
                if(!vis->Visit(tx)) break;
            }
            closedir(dir);
        }

        return vis->VisitEnd();
    }

    bool TransactionPool::Initialize(){
        if(IsInitialized()){
            POOL_WARNING("cannot re-initialize the transaction pool.");
            return false;
        }

        LOG(INFO) << "initializing the transaction pool in: " << GetDataDirectory();
        SetState(TransactionPool::kInitializing);
        if(!FileExists(GetDataDirectory())){
            if(!CreateDirectory(GetDataDirectory())){
                std::stringstream ss;
                ss << "couldn't create the unclaimed transaction pool directory: " << GetDataDirectory();
                POOL_ERROR(ss.str());
                return false;
            }
        }

        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, GetIndexFilename(), &index_).ok()){
            std::stringstream ss;
            ss << "couldn't initialize the transaction pool index: " << GetIndexFilename();
            POOL_ERROR(ss.str());
            return false;
        }

        SetState(TransactionPool::kInitialized);
        POOL_OK("transaction pool initialized.");
        return true;
    }

    bool TransactionPool::HasTransaction(const Hash& hash){
        leveldb::ReadOptions options;
        std::string key = hash.HexString();
        std::string filename;

        LOCK_GUARD;
        leveldb::Status status = GetIndex()->Get(options, key, &filename);
        if(!status.ok()){
            std::stringstream ss;
            ss << "couldn't find transaction " << hash << ": " << status.ToString();
            POOL_WARNING(ss.str());
            return false;
        }
        return true;
    }

    Transaction* TransactionPool::GetTransaction(const Hash& hash){
        leveldb::ReadOptions options;
        std::string key = hash.HexString();
        std::string filename;

        LOCK_GUARD;
        leveldb::Status status = GetIndex()->Get(options, key, &filename);
        if(!status.ok()){
            std::stringstream ss;
            ss << "couldn't find transaction " << hash << " in index: " << status.ToString();
            POOL_WARNING(ss.str());
            return nullptr;
        }

        Transaction* tx = Transaction::NewInstance(filename);
        if(hash != tx->GetHash()){
            std::stringstream ss;
            ss << "couldn't verify transaction hash: " << hash;
            POOL_WARNING(ss.str());
            //TODO: better validation + error handling for UnclaimedTransaction data
            return nullptr;
        }
        return tx;
    }

    bool TransactionPool::RemoveTransaction(const Hash& hash){
        if(!HasTransaction(hash)){
            std::stringstream ss;
            ss << "cannot remove non-existing unclaimed transaction: " << hash;
            POOL_WARNING(ss.str());
            return false;
        }

        leveldb::WriteOptions options;
        options.sync = true;
        std::string key = hash.HexString();
        std::string filename = GetDataFilename(hash);

        leveldb::Status status = GetIndex()->Delete(options, key);
        if(!status.ok()){
            std::stringstream ss;
            ss << "couldn't remove transaction " << hash << " from pool: " << status.ToString();
            POOL_ERROR(ss.str());
            return false;
        }

        if(!DeleteFile(filename)){
            std::stringstream ss;
            ss << "couldn't remove the transaction file: " << filename;
            POOL_ERROR(ss.str());
            return false;
        }

        LOG(INFO) << "removed transaction: " << hash;
        return true;
    }

    bool TransactionPool::PutTransaction(const Hash& hash, Transaction* tx){
        leveldb::WriteOptions options;
        options.sync = true;
        std::string key = hash.HexString();
        std::string filename = GetNewDataFilename(hash);

        LOCK_GUARD;
        if(FileExists(filename)){
            std::stringstream ss;
            ss << "cannot overwrite existing transaction pool data: " << filename;
            POOL_ERROR(ss.str());
            return false;
        }

        if(!tx->WriteToFile(filename)){
            std::stringstream ss;
            ss << "cannot write transaction pool data to: " << filename;
            POOL_ERROR(ss.str());
            return false;
        }

        leveldb::Status status = GetIndex()->Put(options, key, filename);
        if(!status.ok()){
            std::stringstream ss;
            ss << "couldn't index transaction " << tx << ": " << status.ToString();
            POOL_ERROR(ss.str());
            return false;
        }
        LOG(INFO) << "added transaction to pool: " << hash;
        return true;
    }

    bool TransactionPool::GetTransactions(std::vector<Hash>& txs){
        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;

                Transaction* tx = Transaction::NewInstance(filename);
                txs.push_back(tx->GetHash());
            }
            closedir(dir);
            return true;
        }
        return false;
    }

    class TransactionPrinter : public TransactionPoolVisitor{
    public:
        bool Visit(Transaction* tx){
            LOG(INFO) << " - " << tx->GetHash();
            return true;
        }
    };

    bool TransactionPool::Print(){
        TransactionPrinter printer;
        return Accept(&printer);
    }

    size_t TransactionPool::GetSize(){
        size_t size = 0;

        LOCK_GUARD;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(GetDataDirectory().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (GetDataDirectory() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                size++;
            }
            closedir(dir);
        }
        return size;
    }
}