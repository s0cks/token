#include <glog/logging.h>
#include <cryptopp/pssr.h>
#include "common.h"
#include "keychain.h"
#include "block_chain.h"
#include "transaction.h"

namespace Token{
    Input::Input(const UnclaimedTransaction& utxo):
        previous_hash_(utxo.GetTransaction()),
        index_(utxo.GetIndex()){}

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

        Proto::BlockChain::Transaction raw;
        raw << (*GetTransaction());

        size_t size = raw.ByteSizeLong();
        uint8_t bytes[size];
        if(!raw.SerializeToArray(bytes, size)){
            LOG(ERROR) << "couldn't get transaction bytes";
            return false;
        }
        return verifier.VerifyMessage(bytes, size, sigData, sigData.size());
    }

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

    bool TransactionPool::GetTransaction(const uint256_t& hash, Transaction* result){
        TransactionPool* pool = GetInstance();
        if(!pool->LoadObject(hash, result)) return false;
        //TODO: rwlock
        return true;
    }

    bool TransactionPool::RemoveTransaction(const uint256_t& hash){
        TransactionPool* pool = GetInstance();
        //TODO: rwlock
        if(!pool->DeleteObject(hash)){
            return false;
        }
        return true;
    }

    bool TransactionPool::PutTransaction(Transaction* tx){
        TransactionPool* pool = GetInstance();
        //TODO: rwlock
        uint256_t hash = tx->GetHash();
        if(!pool->SaveObject(hash, tx)){
            return false;
        }
        return true;
    }
}