#include <glog/logging.h>
#include <sys/stat.h>
#include <stdio.h>
#include <dirent.h>
#include <blockchain.h>
#include "transaction.h"

namespace Token{
    void Transaction::Accept(Token::TransactionVisitor* vis) const{
        int i;
        for(i = 0; i < GetNumberOfInputs(); i++){
            vis->VisitInput(GetInputAt(i));
        }
        for(i = 0; i < GetNumberOfOutputs(); i++){
            vis->VisitOutput(GetOutputAt(i));
        }
    }

    void Transaction::Encode(Token::ByteBuffer *bb){
        uint32_t size = static_cast<uint32_t>(GetRaw()->ByteSizeLong());
        bb->Resize(size);
        GetRaw()->SerializeToArray(bb->GetBytes(), size);
    }

    std::string Transaction::GetHash(){
        CryptoPP::SHA256 func;
        std::string digest;
        size_t size = GetRaw()->ByteSizeLong();
        uint8_t bytes[size];
        GetRaw()->SerializeToArray(bytes, size);
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    HashArray Transaction::GetHashArray(){
        HashArray result;
        CryptoPP::SHA256 func;
        size_t size = GetRaw()->ByteSizeLong();
        uint8_t bytes[size];
        GetRaw()->SerializeToArray(bytes, size);
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::ArraySink(result.data(), DIGEST_SIZE)));
        return result;
    }

    void Input::Encode(ByteBuffer* bb) const{
        uint32_t size = static_cast<uint32_t>(GetRaw()->ByteSizeLong());
        bb->Resize(size);
        GetRaw()->SerializeToArray(bb->GetBytes(), size);
    }

    std::string Input::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        ByteBuffer bb;
        Encode(&bb);
        CryptoPP::ArraySource source(bb.GetBytes(), bb.Size(), true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    void Output::Encode(Token::ByteBuffer *bb) const{
        uint32_t size = static_cast<uint32_t>(GetRaw()->ByteSizeLong());
        bb->Resize(size);
        GetRaw()->SerializeToArray(bb->GetBytes(), size);
    }

    std::string Output::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        ByteBuffer bb;
        Encode(&bb);
        CryptoPP::ArraySource source(bb.GetBytes(), bb.Size(), true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    static inline bool
    FileExists(const std::string& name){
        std::ifstream f(name.c_str());
        return f.good();
    }

    static inline bool
    CreateTransactionPoolDirectory(const std::string& path){
        return (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) != -1;
    }

    std::string TransactionPool::root_ = "";
    uint32_t TransactionPool::counter_ = 0;

    bool TransactionPool::Initialize(const std::string& path){
        std::string txpool = (path + "/txpool");
        if(!FileExists(txpool)){
            LOG(WARNING) << "transaction pool directory not found, creating...";
            if(!CreateTransactionPoolDirectory(txpool)){
                LOG(ERROR) << "cannot create transaction pool in directory: " << path;
                return false;
            }
            LOG(WARNING) << "done!";
        }
        LOG(INFO) << "initializing transaction pool in: " << txpool;
        root_ = txpool;
        return true;
    }

    bool TransactionPool::SaveTransaction(const std::string &filename, Token::Transaction *tx){
        std::fstream fd(filename, std::ios::binary|std::ios::out|std::ios::trunc);
        if(!tx->GetRaw()->SerializeToOstream(&fd)){
            LOG(ERROR) << "couldn't write transaction to file: " << filename;
            return false;
        }
        LOG(INFO) << "saved transaction: " << (*tx);
        fd.close();
        return true;
    }

    bool TransactionPool::AddTransaction(Token::Transaction* tx){
        if((counter_ + 1) > TransactionPool::kTransactionPoolMaxSize){
            LOG(WARNING) << "transaction pool full";
            return false;
        }
        std::stringstream txfile;
        txfile << root_ << "/tx" << counter_ << ".dat";
        LOG(INFO) << "saving transaction#" << counter_ << " to " << txfile.str();
        if(!SaveTransaction(txfile.str(), tx)){
            return false;
        }
        counter_++;
        return true;
    }

    Transaction* TransactionPool::LoadTransaction(const std::string& filename){
        std::fstream fd(filename, std::ios::binary|std::ios::in);
        Transaction* tx = new Transaction();
        tx->GetRaw()->ParseFromIstream(&fd);
        LOG(INFO) << "loaded transaction from: " << filename << ": " << (*tx);
        fd.close();
        return tx;
    }

    inline bool EndsWith(const std::string& value, const std::string& ending){
        if (ending.size() > value.size()) return false;
        return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
    }

    inline bool ShouldLoadTransaction(const std::string& txfile){
        return EndsWith(txfile, ".dat");
    }

    bool TransactionPool::GetTransactions(std::vector<Transaction*>& txs){
        struct dirent* entry;
        DIR* dir = opendir(root_.c_str());
        if(dir == NULL) return false;
        while((entry = readdir(dir)) != NULL){
            std::string txfile(entry->d_name);
            std::string txfilename = (root_ + "/" + txfile);
            if(ShouldLoadTransaction(txfile)){
                LOG(INFO) << "loading transaction: " << txfilename;
                txs.push_back(LoadTransaction(txfilename));
            }
        }
        closedir(dir);
        return txs.size() > 0;
    }

    Block* TransactionPool::CreateBlock(){
        LOG(INFO) << "getting transactions...";
        std::vector<Transaction*> transactions;
        if(!TransactionPool::GetTransactions(transactions)){
            LOG(ERROR) << "couldn't get transactions from pool";
            return nullptr;
        }
        LOG(INFO) << "creating block from " << transactions.size() << " transactions...";
        Block* block = BlockChain::GetInstance()->CreateBlock();
        for(int i = 0; i < transactions.size(); i++){
            Transaction* tx = transactions[i];
            LOG(INFO) << "appending transaction: " << (*tx);
            if(!block->AppendTransaction(tx)){
                LOG(ERROR) << "couldn't append transaction: " << tx->GetHash();
            }
            delete tx;
        }
        LOG(INFO) << "created new block: " << block->GetHash();
        return block;
    }
}