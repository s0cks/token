#include "keychain.h"
#include "block_chain.h"

namespace Token{
    Input::Input(const UnclaimedTransaction& utxo):
            previous_hash_(utxo.GetTransaction()),
            index_(utxo.GetIndex()){}

    bool Input::GetTransaction(Transaction* result) const{
        return BlockChain::GetTransaction(GetTransactionHash(), result);
    }

    bool Input::GetBytes(CryptoPP::SecByteBlock& bytes) const{
        Proto::BlockChain::Input raw;
        raw << (*this);
        bytes.resize(raw.ByteSizeLong());
        return raw.SerializeToArray(bytes.data(), bytes.size());
    }

    bool Output::GetBytes(CryptoPP::SecByteBlock& bytes) const{
        Proto::BlockChain::Output raw;
        raw << (*this);
        bytes.resize(raw.ByteSizeLong());
        return raw.SerializeToArray(bytes.data(), bytes.size());
    }

    bool Transaction::GetBytes(CryptoPP::SecByteBlock& bytes) const{
        Proto::BlockChain::Transaction raw;
        raw << (*this);
        bytes.resize(raw.ByteSizeLong());
        return raw.SerializeToArray(bytes.data(), bytes.size());
    }

    bool Transaction::Accept(Token::TransactionVisitor* vis){
        if(!vis->VisitStart()) return false;
        // visit the inputs
        {
            if(!vis->VisitInputsStart()) return false;
            for(uint64_t idx = 0; idx < GetNumberOfInputs(); idx++){
                Input input;
                if(!GetInput(idx, &input)) return false;
                if(!vis->VisitInput(input)) return false;
            }
            if(!vis->VisitInputsEnd()) return false;
        }
        // visit the outputs
        {
            if(!vis->VisitOutputsStart()) return false;
            for(uint64_t idx = 0; idx < GetNumberOfOutputs(); idx++){
                Output output;
                if(!GetOutput(idx, &output)) return false;
                if(!vis->VisitOutput(output)) return false;
            }
            if(!vis->VisitOutputsEnd()) return false;
        }
        return vis->VisitEnd();
    }

    bool Transaction::Sign(){
        CryptoPP::SecByteBlock bytes;
        if(!GetBytes(bytes)){
            LOG(ERROR) << "couldn't serialize transaction to byte array";
            return false;
        }

        CryptoPP::RSA::PrivateKey privateKey;
        CryptoPP::RSA::PublicKey publicKey;
        if(!TokenKeychain::LoadKeys(&privateKey, &publicKey)){
            LOG(ERROR) << "couldn't load token keychain";
            return false;
        }

        try{
            LOG(INFO) << "signing transaction: " << HexString(GetHash());
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

#define READ_LOCK pthread_rwlock_tryrdlock(&pool->rwlock_)
#define WRITE_LOCK pthread_rwlock_trywrlock(&pool->rwlock_)
#define UNLOCK pthread_rwlock_unlock(&pool->rwlock_)

    TransactionPool* TransactionPool::GetInstance(){
        static TransactionPool instance;
        return &instance;
    }

    bool TransactionPool::Initialize(){
        TransactionPool* pool = GetInstance();
        if(!FileExists(pool->GetRoot())){
            if(!CreateDirectory(pool->GetRoot())) return false;
        }
        return pool->InitializeIndex();
    }

    bool TransactionPool::HasTransaction(const uint256_t& hash){
        TransactionPool* pool = GetInstance();
        READ_LOCK;
        bool found = pool->ContainsObject(hash);
        UNLOCK;
        return found;
    }

    Transaction* TransactionPool::GetTransaction(const uint256_t& hash){
        TransactionPool* pool = GetInstance();
        READ_LOCK;
        Transaction* result = pool->LoadObject(hash);
        UNLOCK;
        return result;
    }

    bool TransactionPool::GetTransactions(std::vector<uint256_t>& txs){
        TransactionPool* pool = GetInstance();
        READ_LOCK;
        DIR* dir;
        struct dirent* ent;
        if((dir = opendir(pool->GetRoot().c_str())) != NULL){
            while((ent = readdir(dir)) != NULL){
                std::string name(ent->d_name);
                std::string filename = (pool->GetRoot() + "/" + name);
                if(!EndsWith(filename, ".dat")) continue;
                Transaction* tx;
                if(!(tx = pool->LoadRawObject(filename))){
                    LOG(ERROR) << "couldn't load transaction from: " << filename;
                    return false;
                }
                txs.push_back(tx->GetHash());
                delete tx;
            }
            closedir(dir);
            UNLOCK;
            return true;
        }
        UNLOCK;
        return false;
    }

    bool TransactionPool::RemoveTransaction(const uint256_t& hash){
        TransactionPool* pool = GetInstance();
        WRITE_LOCK;
        if(!pool->DeleteObject(hash)){
            UNLOCK;
            return false;
        }
        UNLOCK;
        return true;
    }

    bool TransactionPool::PutTransaction(Transaction* tx){
        TransactionPool* pool = GetInstance();
        WRITE_LOCK;
        uint256_t hash = tx->GetHash();
        if(!pool->SaveObject(hash, tx)){
            UNLOCK;
            return false;
        }
        UNLOCK;
        return true;
    }

    uint64_t TransactionPool::GetSize(){
        TransactionPool* pool = GetInstance();
        READ_LOCK;
        uint64_t size = 0;
        DIR* dir;
        if((dir = opendir(pool->GetRoot().c_str())) != nullptr){
            struct dirent* ent;
            while((ent = readdir(dir))){
                std::string name(ent->d_name);
                if(EndsWith(name, ".dat")) size++;
            }
        }
        UNLOCK;
        return size;
    }
}