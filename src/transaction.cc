#include "transaction.h"
#include "keychain.h"
#include "block_chain.h"
#include "crash_report.h"
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
        Transaction* instance = (Transaction*)Allocator::Allocate(sizeof(Transaction), Object::kTransaction);
        new (instance)Transaction(timestamp, index, inputs, outputs);
        return instance;
    }

    Transaction* Transaction::NewInstance(const Transaction::RawType& raw){
        Transaction* instance = (Transaction*)Allocator::Allocate(sizeof(Transaction), Object::kTransaction);
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

    Transaction* Transaction::NewInstance(std::fstream& fd){
        Transaction::RawType raw;
        if(!raw.ParseFromIstream(&fd)) return nullptr;
        return NewInstance(raw);
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
        Keychain::LoadKeys(&privateKey, &publicKey);

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
    static std::recursive_mutex mutex_;
    static TransactionPool::State state_;
    static leveldb::DB* index_ = nullptr;

#define LOCK_GUARD std::lock_guard<std::recursive_mutex> guard(mutex_);

    static inline std::string
    GetDataDirectory(){
        return FLAGS_path + "/txs";
    }

    static inline std::string
    GetIndexFilename(){
        return GetDataDirectory() + "/index";
    }

    static inline leveldb::DB*
    GetIndex(){
        return index_;
    }

    static inline std::string
    GetNewTransactionFilename(const uint256_t& hash){
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
    GetTransactionFilename(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = HexString(hash);
        std::string filename;
        if(!GetIndex()->Get(options, key, &filename).ok()) return GetNewTransactionFilename(hash);
        return filename;
    }

    TransactionPool::State TransactionPool::GetState(){
        LOCK_GUARD;
        return state_;
    }

    void TransactionPool::SetState(TransactionPool::State state){
        LOCK_GUARD;
        state_ = state;
    }

    void TransactionPool::Initialize(){
        if(!IsUninitialized()){
            std::stringstream ss;
            ss << "Cannot reinitialize the transaction pool";
            CrashReport::GenerateAndExit(ss);
        }

        SetState(State::kInitializing);
        std::string filename = GetDataDirectory();
        if(!FileExists(filename)){
            if(!CreateDirectory(filename)){
                std::stringstream ss;
                ss << "Cannot create the TransactionPool data directory: " << filename;
                CrashReport::GenerateAndExit(ss);
            }
        }

        leveldb::Options options;
        options.create_if_missing = true;
        if(!leveldb::DB::Open(options, GetIndexFilename(), &index_).ok()){
            std::stringstream ss;
            ss << "Cannot create TransactionPool index: " << GetIndexFilename();
        }

        SetState(State::kInitialized);
#ifdef TOKEN_DEBUG
        LOG(INFO) << "initialized the transaction pool";
#endif//TOKEN_DEBUG
    }

    bool TransactionPool::HasTransaction(const uint256_t& hash){
        leveldb::ReadOptions options;
        std::string key = HexString(hash);
        std::string filename;
        LOCK_GUARD;
        return GetIndex()->Get(options, key, &filename).ok();
    }

    Transaction* TransactionPool::GetTransaction(const uint256_t& hash){
        if(!HasTransaction(hash)) return nullptr;
        LOCK_GUARD;
        std::string filename = GetTransactionFilename(hash);
        return Transaction::NewInstance(filename);
    }

    bool TransactionPool::GetTransactions(std::vector<uint256_t>& txs){
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

    void TransactionPool::RemoveTransaction(const uint256_t& hash){
        if(!HasTransaction(hash)) return;
        LOCK_GUARD;
        std::string filename = GetTransactionFilename(hash);
        if(!DeleteFile(filename)){
            std::stringstream ss;
            ss << "Couldn't remove transaction " << hash << " data file: " << filename;
            CrashReport::GenerateAndExit(ss);
        }

        leveldb::WriteOptions options;
        if(!GetIndex()->Delete(options, HexString(hash)).ok()){
            std::stringstream ss;
            ss << "Couldn't remove transaction " << hash << " from index";
            CrashReport::GenerateAndExit(ss);
        }
    }

    void TransactionPool::PutTransaction(Transaction* tx){
        uint256_t hash = tx->GetHash();
        if(HasTransaction(hash)){
            std::stringstream ss;
            ss << "Couldn't add duplicate transaction " << hash << " to transaction pool";
            CrashReport::GenerateAndExit(ss);
        }

        LOCK_GUARD;
        std::string filename = GetNewTransactionFilename(hash);
        std::fstream fd(filename, std::ios::out|std::ios::binary);
        if(!tx->WriteToFile(fd)){
            std::stringstream ss;
            ss << "Couldn't write transaction " << hash << " to file: " << filename;
            CrashReport::GenerateAndExit(ss);
        }

        leveldb::WriteOptions options;
        std::string key = HexString(hash);
        if(!GetIndex()->Put(options, key, filename).ok()){
            std::stringstream ss;
            ss << "Couldn't index transaction: " << hash;
            CrashReport::GenerateAndExit(ss);
        }
    }

    uint32_t TransactionPool::GetNumberOfTransactions(){
        std::vector<uint256_t> txs;
        if(!GetTransactions(txs)) return 0;
        return txs.size();
    }
}