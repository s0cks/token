#include <glog/logging.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <dirent.h>
#include <cryptopp/pssr.h>
#include <block_miner.h>

#include "common.h"
#include "keychain.h"
#include "block_chain.h"
#include "transaction.h"

namespace Token{
    void Transaction::Accept(Token::TransactionVisitor* vis){
        if(!vis->VisitStart()) return;

        vis->VisitInputsStart();
        int i;
        for(i = 0; i < GetNumberOfInputs(); i++){
            Input* in;
            if(!(in = GetInputAt(i))) return;
            if(!vis->VisitInput(in)) return;
        }
        vis->VisitInputsEnd();

        vis->VisitOutputsStart();
        for(i = 0; i < GetNumberOfOutputs(); i++){
            Output* out;
            if(!(out = GetOutputAt(i))) return;
            if(!vis->VisitOutput(out)) return;
        }
        vis->VisitOutputsEnd();

        if(!vis->VisitEnd()) return;
    }

    void Transaction::SetTimestamp(){
        //TODO: implement
    }

    void Transaction::SetIndex(uint32_t index){
        index_ = index;
    }

    void Transaction::SetSignature(std::string signature){
        //TODO: implement
    }

    std::string Transaction::GetSignature() const{
        //TODO: implement
        return "";
    }

    std::string Transaction::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        Messages::Transaction raw;
        raw << this;
        size_t size = raw.ByteSizeLong();
        uint8_t bytes[size];
        if(!raw.SerializeToArray(bytes, size)){
            LOG(WARNING) << "couldn't serialize transaction to byte array";
            return "";
        }
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    std::string Input::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        Messages::Input raw;
        raw << this;
        size_t size = raw.ByteSizeLong();
        uint8_t bytes[size];
        if(!raw.SerializeToArray(bytes, size)) {
            LOG(WARNING) << "couldn't serialize input to byte array";
            return "";
        }
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    std::string Output::GetHash() const{
        CryptoPP::SHA256 func;
        std::string digest;
        Messages::Output output;
        output << this;
        size_t size = output.ByteSizeLong();
        uint8_t bytes[size];
        if(!output.SerializePartialToArray(bytes, size)){
            LOG(WARNING) << "couldn't serialize output to byte array";
            return "";
        }
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    static inline bool
    CreateTransactionPoolDirectory(const std::string& path){
        return (mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) != -1;
    }

    uint32_t TransactionPool::counter_ = 0;

    bool TransactionPool::Initialize(){
        std::string txpool = (TOKEN_BLOCKCHAIN_HOME + "/tx");
        if(!FileExists(txpool)){
            LOG(WARNING) << "transaction pool directory not found, creating...";
            if(!CreateTransactionPoolDirectory(txpool)){
                LOG(ERROR) << "cannot create transaction pool: " << txpool;
                return false;
            }
            LOG(WARNING) << "done!";
        }
        LOG(INFO) << "initializing transaction pool in: " << txpool;
        return true;
    }

    bool TransactionPool::SaveTransaction(const std::string &filename, Token::Transaction *tx){
        std::fstream fd(filename, std::ios::binary|std::ios::out|std::ios::trunc);
        Messages::Transaction raw;
        raw << tx;
        if(!raw.SerializeToOstream(&fd)){
            LOG(ERROR) << "couldn't write transaction to file: " << filename;
            return false;
        }
        LOG(INFO) << "saved transaction:";
        fd.close();
        return true;
    }

    bool TransactionPool::AddTransaction(Token::Transaction* tx){
        if((counter_ + 1) > TransactionPool::kTransactionPoolMaxSize){
            LOG(WARNING) << "transaction pool full, creating block...";
            if(!CreateBlock()){
                LOG(ERROR) << "couldn't create block from transaction pool transactions";
                return false;
            }
            // Message msg(Message::Type::kBlockMessage, block->GetAsMessage());
            //TODO: BlockChainServer::AsyncBroadcast(&msg); //TODO: Check side, AsyncBroadcast may not work on libuv threads
            return AddTransaction(tx);
        }
        std::stringstream txfile;
        txfile << TOKEN_BLOCKCHAIN_HOME << "/tx/tx" << counter_ << ".dat";
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

    bool TransactionPool::LoadTransaction(const std::string &filename, Token::Messages::Transaction *result,
                                          bool deleteAfter){
        std::fstream fd(filename, std::ios::binary|std::ios::in);
        if(!result->ParseFromIstream(&fd)){
            LOG(ERROR) << "couldn't parse transaction from file: " << filename;
            return false;
        }
        fd.close();

        LOG(INFO) << "loaded transaction: " << filename;
        if(deleteAfter){
            if(!DeleteTransactionFile(filename)){
                LOG(ERROR) << "couldn't delete transaction pool file: " << filename;
                return false;
            }
        }
        return true;
    }

    bool TransactionPool::CreateBlock(){
        LOG(INFO) << "creating block from " << counter_ << " transactions...";
        Messages::Block nblock;
        nblock.set_previous_hash(BlockChain::GetHead()->GetHash());
        nblock.set_height(BlockChain::GetHead()->GetHeight() + 1);
        for(size_t idx = 0; idx < counter_; idx++){
            std::stringstream filename;
            filename << TOKEN_BLOCKCHAIN_HOME << "/tx/tx" << idx << ".dat";
            if(!FileExists(filename.str()) || !LoadTransaction(filename.str(), nblock.add_transactions())){
                return false;
            }
        }
        counter_ = 0;
        return BlockMiner::ScheduleRawBlock(nblock);
    }

    bool TransactionSigner::GetSignature(std::string* signature){
        Messages::Transaction raw;
        raw << GetTransaction();
        size_t size = raw.ByteSizeLong();
        uint8_t bytes[size];
        if(!raw.SerializeToArray(bytes, size)){
            LOG(ERROR) << "couldn't serialize transaction to byte array";
            return false;
        }
        CryptoPP::RSA::PrivateKey privateKey;
        CryptoPP::RSA::PublicKey publicKey;
        if(!TokenKeychain::LoadKeys(&privateKey, &publicKey)){
            LOG(WARNING) << "couldn't load chain keys";

            free(bytes);
            return false;
        }

        try{
            LOG(INFO) << "signing transaction: " << GetTransaction()->GetHash();
            CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA256>::Signer signer(privateKey);
            CryptoPP::AutoSeededRandomPool rng;

            CryptoPP::SecByteBlock sigData(signer.MaxSignatureLength());
            size_t length = signer.SignMessage(rng, bytes, size, sigData);
            sigData.resize(length);

            CryptoPP::ArraySource source(sigData.data(), sigData.size(), true, new CryptoPP::HexEncoder(new CryptoPP::StringSink(*signature)));

            free(bytes);
            return true;
        } catch(CryptoPP::Exception& ex){
            LOG(ERROR) << "error occurred signing transaction: " << ex.GetWhat();
            return false;
        }
    }

    bool TransactionSigner::Sign(){
        std::string signature;
        if(!GetSignature(&signature)){
            LOG(ERROR) << "couldn't generate signature";
            return false;
        }
        GetTransaction()->SetSignature(signature);
        return true;
    }

    bool TransactionVerifier::VerifySignature() {
        CryptoPP::RSA::PrivateKey privateKey;
        CryptoPP::RSA::PublicKey publicKey;
        if (!TokenKeychain::LoadKeys(&privateKey, &publicKey)) {
            LOG(ERROR) << "couldn't load keys";
            return false;
        }

        CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA256>::Signer signer;

        std::string sigstr = GetTransaction()->GetSignature();
        CryptoPP::SecByteBlock sigData(signer.MaxSignatureLength());
        CryptoPP::StringSource source(sigstr, true, new CryptoPP::HexDecoder(
                new CryptoPP::ArraySink(sigData.data(), sigData.size())));

        CryptoPP::RSASS<CryptoPP::PSS, CryptoPP::SHA256>::Verifier verifier(publicKey);

        Messages::Transaction raw;
        raw << GetTransaction();

        size_t size = raw.ByteSizeLong();
        uint8_t bytes[size];
        if(!raw.SerializeToArray(bytes, size)){
            LOG(ERROR) << "couldn't get transaction bytes";
            return false;
        }
        return verifier.VerifyMessage(bytes, size, sigData, sigData.size());
    }
}