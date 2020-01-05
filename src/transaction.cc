#include <glog/logging.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <stdio.h>
#include <dirent.h>
#include <cryptopp/pssr.h>
#include <cryptopp/whrlpool.h>
#include <node/message.h>

#include "allocator.h"
#include "blockchain.h"
#include "transaction.h"

namespace Token{
    void Transaction::Accept(Token::TransactionVisitor* vis){
        int i;
        for(i = 0; i < GetNumberOfInputs(); i++){
            vis->VisitInput(GetInputAt(i));
        }
        for(i = 0; i < GetNumberOfOutputs(); i++){
            vis->VisitOutput(GetOutputAt(i));
        }
    }

    void Transaction::SetTimestamp(){
        GetRaw()->set_timestamp(GetCurrentTime());
    }

    typedef CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::Whirlpool>::Signer Signer;
    typedef CryptoPP::RSASS<CryptoPP::PSSR, CryptoPP::Whirlpool>::Verifier Verifier;

    bool Transaction::Sign(CryptoPP::RSA::PrivateKey key){
        size_t size = GetRaw()->ByteSizeLong();
        uint8_t bytes[size];
        GetRaw()->SerializeToArray(bytes, size);
        std::string digest;
        Signer signer(key);
        CryptoPP::AutoSeededRandomPool prng;
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::SignerFilter(prng, signer, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        GetRaw()->set_signature(digest);
        return true;
    }

    std::string Transaction::GetSignature(){
        return GetRaw()->signature();
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

    std::string Transaction::ToString(){
        std::stringstream stream;
        stream << "Transaction(" << GetHash() << ")";
        return stream.str();
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

    std::string Input::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        size_t size = GetRaw()->ByteSizeLong();
        uint8_t bytes[size];
        GetRaw()->SerializeToArray(bytes, size);
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    std::string Output::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        size_t size = GetRaw()->ByteSizeLong();
        uint8_t bytes[size];
        GetRaw()->SerializeToArray(bytes, size);
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
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
            LOG(WARNING) << "transaction pool full, creating block...";
            Block* block = CreateBlock();
            if(!BlockChain::Append(block)){
                LOG(ERROR) << "couldn't append new block: " << block->GetHash();
                return false;
            }
            // Message msg(Message::Type::kBlockMessage, block->GetAsMessage());
            //TODO: BlockChainServer::AsyncBroadcast(&msg); //TODO: Check side, AsyncBroadcast may not work on libuv threads
            return true;
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

    static inline bool
    DeleteTransactionFile(const std::string& filename){
        int rc;
        if((rc = remove(filename.c_str())) == 0){
            return true;
        }
        LOG(WARNING) << "unable to delete file: " << filename;
        return false;
    }

    Transaction* TransactionPool::LoadTransaction(const std::string& filename){
        std::fstream fd(filename, std::ios::binary|std::ios::in);
        Transaction* tx = new Transaction();
        if(!tx->GetRaw()->ParseFromIstream(&fd)){
            LOG(ERROR) << "couldn't parse transaction from file: " << filename;
            return nullptr;
        }
        fd.close();
        if(!DeleteTransactionFile(filename)){
            LOG(ERROR) << "couldn't delete transaction pool file: " << filename;
            return nullptr;
        }
        LOG(INFO) << "loaded transaction: " << tx->ToString();
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
        Block* block = new Block(BlockChain::GetHead());
        for(int i = 0; i < transactions.size(); i++){
            Transaction* tx = transactions[i];
            LOG(INFO) << "appending transaction: " << (*tx);
            if(!block->AppendTransaction(tx)){
                LOG(ERROR) << "couldn't append transaction: " << tx->GetHash();
            }
        }
        counter_ = 0;
        LOG(INFO) << "created new block: " << block->GetHash();
        return block;
    }
}