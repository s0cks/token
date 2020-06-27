#include "transaction.h"

#include "keychain.h"
#include "block_chain.h"
#include "node/node.h"

namespace Token{
//######################################################################################################################
//                                          Input
//######################################################################################################################
    std::string Input::ToString() const{
        std::stringstream stream;
        stream << "Input(" << GetTransactionHash() << "[" << GetOutputIndex() << "]" << ")";
        return stream.str();
    }

    UnclaimedTransaction* Input::GetUnclaimedTransaction() const{
        return UnclaimedTransactionPool::GetUnclaimedTransaction(hash_, index_);
    }

//######################################################################################################################
//                                          Output
//######################################################################################################################
    std::string Output::ToString() const{
        std::stringstream stream;
        stream << "Output(" << GetUser() << "; " << GetToken() << ")";
        return stream.str();
    }
//######################################################################################################################
//                                          Transaction
//######################################################################################################################
    Transaction* Transaction::NewInstance(uint32_t index, Transaction::InputList& inputs, Transaction::OutputList& outputs, uint32_t timestamp){
        Transaction* instance = (Transaction*)Allocator::Allocate(sizeof(Transaction));
        new (instance)Transaction(timestamp, index, inputs, outputs);
        return instance;
    }

    Transaction* Transaction::NewInstance(const Transaction::RawType& raw){
        Transaction* instance = (Transaction*)Allocator::Allocate(sizeof(Transaction));
        Transaction::InputList inputs;
        for(auto& it : raw.inputs()){
            inputs.push_back(Input(it));
        }

        Transaction::OutputList outputs;
        for(auto& it : raw.outputs()){
            outputs.push_back(Output(it));
        }

        new (instance)Transaction(raw.timestamp(), raw.index(), inputs, outputs);
        return instance;
    }

    std::string Transaction::ToString() const{
        std::stringstream stream;
        stream << "Transaction(" << GetHash() << ")";
        return stream.str();
    }

    bool Transaction::Encode(Transaction::RawType& raw) const{
        raw.set_timestamp(timestamp_);
        raw.set_index(index_);
        raw.set_signature(signature_);
        for(auto& it : inputs_){
            Input::MessageType* raw_in = raw.add_inputs();
            raw_in->set_index(it.GetOutputIndex());
            raw_in->set_previous_hash(HexString(it.GetTransactionHash()));
            raw_in->set_user(it.GetUser());
        }
        for(auto& it : outputs_){
            Output::MessageType* raw_out = raw.add_outputs();
            raw_out->set_user(it.GetUser());
            raw_out->set_token(it.GetToken());
        }
        return true;
    }

    bool Transaction::Accept(Token::TransactionVisitor* vis){
        if(!vis->VisitStart()) return false;
        // visit the inputs
        {
            if(!vis->VisitInputsStart()) return false;
            for(uint64_t idx = 0; idx < GetNumberOfInputs(); idx++){
                Input input;
                if(!GetInput(idx, &input)) return false;
                if(!vis->VisitInput(&input)) return false;
            }
            if(!vis->VisitInputsEnd()) return false;
        }
        // visit the outputs
        {
            if(!vis->VisitOutputsStart()) return false;
            for(uint64_t idx = 0; idx < GetNumberOfOutputs(); idx++){
                Output output;
                if(!GetOutput(idx, &output)) return false;
                if(!vis->VisitOutput(&output)) return false;
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
        if(!Keychain::LoadKeys(&privateKey, &publicKey)){
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
//######################################################################################################################
//                                          Transaction Pool
//######################################################################################################################

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

    bool TransactionPool::AddTransaction(Transaction* tx){
        TransactionPool* pool = GetInstance();
        WRITE_LOCK;
        uint256_t hash = tx->GetHash();
        if(!pool->SaveObject(hash, tx)){
            UNLOCK;
            return false;
        }

        //Node::BroadcastInventory(tx);
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